/**************************************************************************
 *   Created: 2012/07/09 16:10:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Dispatcher.hpp"
#include "Core/Algo.hpp"
#include "Core/Observer.hpp"
#include "Core/Security.hpp"
#include "Core/PositionBundle.hpp"
#include "Core/Position.hpp"
#include "Core/PositionReporter.hpp"
#include "Core/Settings.hpp"

namespace mi = boost::multi_index;
namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Engine;

//////////////////////////////////////////////////////////////////////////

class Dispatcher::AlgoState
		: private boost::noncopyable,
		public boost::enable_shared_from_this<AlgoState> {

private:

	typedef SignalConnectionList<Position::StateUpdateConnection> StateUpdateConnections;

public:

	explicit AlgoState(
				boost::shared_ptr<Algo> algo,
				boost::shared_ptr<Notifier> notifier,
				boost::shared_ptr<const Settings> settings)
			: m_algo(algo),
			m_notifier(notifier),
			m_isBlocked(false),
			m_lastUpdate(0),
			m_settings(settings) {
		//...//
	}

	void CheckPositions(bool byTimeout) {
		Assert(!m_settings->IsReplayMode() || !byTimeout);
		if (byTimeout && !IsTimeToUpdate()) {
			return;
		}
		const Algo::Lock lock(m_algo->GetMutex());
		if (m_isBlocked || (byTimeout && !IsTimeToUpdate())) {
			return;
		}
		while (CheckPositionsUnsafe());
	}

	bool IsTimeToUpdate() const {
		if ((IsBlocked() || !m_lastUpdate) && m_settings->ShouldWaitForMarketData()) {
			return false;
		}
		const auto now
			= (boost::get_system_time() - m_settings->GetStartTime()).total_milliseconds();
		AssertGe(now, m_lastUpdate);
		const auto diff = boost::uint64_t(now - m_lastUpdate);
		return diff >= m_settings->GetUpdatePeriodMilliseconds();
	}

	bool IsBlocked() const {
		return m_isBlocked || !m_algo->IsValidPrice(*m_settings);
	}

private:

	bool CheckPositionsUnsafe();

private:

	void ReportClosedPositon(PositionBandle &positions) {
		Assert(!positions.Get().empty());
		foreach (auto &p, positions.Get()) {
			if (	p->IsOpened()
					&& !p->IsReported()
					&& (p->IsClosed() || p->IsError())) {
				m_algo->GetPositionReporter().ReportClosedPositon(*p);
				p->MarkAsReported();
			}
		}
	}

private:

	boost::shared_ptr<Algo> m_algo;
	boost::shared_ptr<PositionBandle> m_positions;
		
	boost::shared_ptr<Notifier> m_notifier;
	StateUpdateConnections m_stateUpdateConnections;

	volatile int64_t m_isBlocked;

	volatile int64_t m_lastUpdate;

	boost::shared_ptr<const Settings> m_settings;

};

//////////////////////////////////////////////////////////////////////////

class Dispatcher::ObserverState
		: private boost::noncopyable,
		public boost::enable_shared_from_this<ObserverState> {

public:

	explicit ObserverState(boost::shared_ptr<Observer> observer)
			: m_observer(observer) {
		//...//
	}

	void NotifyUpdate(
				const Security &security,
				Trader::Security::ScaledPrice price,
				Trader::Security::Qty qty,
				bool isBuy) {
		m_observer->OnUpdate(security, price, qty, isBuy);
	}

private:

	boost::shared_ptr<Observer> m_observer;

};

//////////////////////////////////////////////////////////////////////////

class Dispatcher::Notifier : private boost::noncopyable {

public:

	typedef std::list<boost::shared_ptr<AlgoState>> AlgoStateList;
	typedef std::list<boost::shared_ptr<ObserverState>> ObserversStateList;
	
	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

private:

	typedef boost::condition_variable Condition;

	typedef std::map<boost::shared_ptr<AlgoState>, bool> AlgoNotifyList;

	struct ObservationEvent {
		boost::shared_ptr<ObserverState> state;
		boost::shared_ptr<const Security> security;
		Security::ScaledPrice price;
		Security::Qty qty;
		bool isBuy;
	};
	typedef std::list<ObservationEvent> ObserverNotifyList;

public:

	explicit Notifier(boost::shared_ptr<const Settings> settings)
			: m_isActive(false),
			m_isExit(false),
			m_currentAlgos(&m_algoQueue.first),
			m_currentObservers(&m_observerQueue.first),
			m_settings(settings) {
		
		{
			const char *const threadName = "Algo";
			Log::Info(
				"Starting %1% thread(s) \"%2%\"...",
				m_settings->GetAlgoThreadsCount(),
				threadName);
			boost::barrier startBarrier(m_settings->GetAlgoThreadsCount() + 1);
			for (size_t i = 0; i < m_settings->GetAlgoThreadsCount(); ++i) {
				m_threads.create_thread(
					[&]() {
						Task(startBarrier, threadName, &Dispatcher::Notifier::AlgoIteration);
					});
			}
			startBarrier.wait();
			Log::Info("All \"%1%\" threads started.", threadName);
		}
		{
			boost::barrier startBarrier(1 + 1);
			m_threads.create_thread(
				[&]() {
					Task(startBarrier, "Observer", &Dispatcher::Notifier::ObserverIteration);
				});
			startBarrier.wait();
		}
		if (!m_settings->IsReplayMode()) {
			const size_t threadsCount = 1;
			const char *const threadName = "Timeout";
			Log::Info(
				"Starting %1% thread(s) \"%2%\"...",
				threadsCount,
				threadName);
			boost::barrier startBarrier(threadsCount + 1);
			for (size_t i = 0; i < threadsCount; ++i) {
				m_threads.create_thread(
					[&]() {
						Task(startBarrier, threadName, &Dispatcher::Notifier::TimeoutCheckIteration);
					});
			}
			startBarrier.wait();
			Log::Info("All \"%1%\" threads started.", threadName);
		}
	}

	~Notifier() {
		try {
			{
				const Lock lock(m_mutex);
				Assert(!m_isExit);
				m_isExit = true;
				m_isActive = false;
				m_positionsCheckCompletedCondition.notify_all();
				m_positionsCheckCondition.notify_all();
				m_securityUpdateCondition.notify_all();
			}
			m_threads.join_all();
		} catch (...) {
			AssertFailNoException();
			throw;
		}
	}

	boost::shared_ptr<const Settings> GetSettings() const {
		return m_settings;
	}

public:

	void Start() {
		const Lock lock(m_mutex);
		if (m_isActive) {
			return;
		}
		Log::Info("Starting events dispatching...");
		m_isActive = true;
	}

	void Stop() {
		const Lock lock(m_mutex);
		m_isActive = false;
	}

public:

	Mutex & GetAlgoListMutex() {
		return m_algoListMutex;
	}

	AlgoStateList & GetAlgoList() {
		return m_algoList;
	}

	ObserversStateList & GetObserversStateList() {
		return m_observerList;
	}

	void Signal(boost::shared_ptr<AlgoState> algoState) {
		
		if (algoState->IsBlocked()) {
			return;
		}
		
		Lock lock(m_mutex);
		if (m_isExit) {
			return;
		}

		const AlgoNotifyList::const_iterator i = m_currentAlgos->find(algoState);
		if (i != m_currentAlgos->end() && !i->second) {
			return;
		}
		(*m_currentAlgos)[algoState] = false;
		m_positionsCheckCondition.notify_one();
		if (m_settings->IsReplayMode()) {
			m_positionsCheckCompletedCondition.wait(lock);
		}

	}

	void Signal(
				boost::shared_ptr<ObserverState> observerState,
				const boost::shared_ptr<const Security> &security,
				Security::ScaledPrice price,
				Security::Qty qty,
				bool isBuy) {

		ObservationEvent observationEvent = {};
		observationEvent.state = observerState;
		observationEvent.security = security;
		observationEvent.price = price;
		observationEvent.qty = qty;
		observationEvent.isBuy = isBuy;

		Lock lock(m_mutex);
		if (m_isExit) {
			return;
		}

		m_currentObservers->push_back(observationEvent);
		m_securityUpdateCondition.notify_one();

	}

private:

	template<typename Iteration>
	void Task(
				boost::barrier &startBarrier,
				const char *const name,
				Iteration iteration) {
		startBarrier.wait();
		Log::Info("Dispatcher task \"%1%\" started...", name);
		for ( ; ; ) {
			try {
				if (!(this->*iteration)()) {
					break;
				}
			} catch (...) {
				Log::Error("Unhandled exception caught in dispatcher task \"%1%\".", name);
				AssertFailNoException();
				throw;
			}
		}
		Log::Info("Dispatcher task \"%1%\" stopped.", name);
	}

	bool TimeoutCheckIteration();
	bool AlgoIteration();
	bool ObserverIteration();

private:

	bool m_isActive;
	bool m_isExit;

	Mutex m_mutex;
	Condition m_positionsCheckCondition;
	Condition m_positionsCheckCompletedCondition;

	Condition m_securityUpdateCondition;

	std::pair<AlgoNotifyList, AlgoNotifyList> m_algoQueue;
	AlgoNotifyList *m_currentAlgos;

	std::pair<ObserverNotifyList, ObserverNotifyList> m_observerQueue;
	ObserverNotifyList *m_currentObservers;

	boost::shared_ptr<const Settings> m_settings;

	Mutex m_algoListMutex;
	AlgoStateList m_algoList;
	
	ObserversStateList m_observerList;

	boost::thread_group m_threads;

};

//////////////////////////////////////////////////////////////////////////

bool Dispatcher::AlgoState::CheckPositionsUnsafe() {

	const auto now = boost::get_system_time();
	Interlocking::Exchange(
		m_lastUpdate,
		(now - m_settings->GetStartTime()).total_milliseconds());

	Assert(!m_isBlocked);
		
	const Security &security = *const_cast<const Algo &>(*m_algo).GetSecurity();
	Assert(security || !m_settings->ShouldWaitForMarketData()); // must be checked it security object

	if (m_positions) {
		Assert(!m_positions->Get().empty());
		Assert(m_stateUpdateConnections.IsConnected());
		Assert(!security.IsHistoryData());
		ReportClosedPositon(*m_positions);
		if (!m_positions->IsCompleted()) {
			if (m_positions->IsOk()) {
				if (	!m_settings->IsReplayMode()
						&& m_settings->GetCurrentTradeSessionEndime() <= now) {
					foreach (auto &p, m_positions->Get()) {
						if (p->IsCanceled()) {
							continue;
						}
						p->CancelAtMarketPrice(Position::CLOSE_TYPE_SCHEDULE);
					}
				} else {
					m_algo->TryToClosePositions(*m_positions);
				}
			} else {
				Log::Warn("Algo \"%1%\" BLOCKED by dispatcher.", m_algo->GetName());
				Interlocking::Exchange(m_isBlocked, true);
			}
			return false;
		}
		m_stateUpdateConnections.Diconnect();
		m_positions.reset();
	}

	Assert(!m_stateUpdateConnections.IsConnected());

	if (security.IsHistoryData()) {
		m_algo->Update();
		return false;
	}

	boost::shared_ptr<PositionBandle> positions = m_algo->TryToOpenPositions();
	if (!positions || positions->Get().empty()) {
		return false;
	}

	StateUpdateConnections stateUpdateConnections;
	foreach (const auto &p, positions->Get()) {
		Assert(&p->GetSecurity() == &security);
		m_algo->ReportDecision(*p);
		stateUpdateConnections.InsertSafe(
			p->Subscribe(
				boost::bind(&Notifier::Signal, m_notifier.get(), shared_from_this())));
	}
	Assert(stateUpdateConnections.IsConnected());

	stateUpdateConnections.Swap(m_stateUpdateConnections);
	positions.swap(m_positions);
	return true;

}

//////////////////////////////////////////////////////////////////////////

bool Dispatcher::Notifier::TimeoutCheckIteration() {

	const auto nextIterationTime
		= boost::get_system_time() + pt::milliseconds(m_settings->GetUpdatePeriodMilliseconds());

	std::list<boost::shared_ptr<AlgoState>> algos;
	{
		const Lock lock(m_algoListMutex);
		foreach (boost::shared_ptr<AlgoState> &algo, m_algoList) {
			if (algo->IsTimeToUpdate()) {
				algos.push_back(algo);
			}
		}
	}
	bool result = true;
	{
		Lock lock(m_mutex);
		if (m_isExit) {
			result = false;
		} else if (m_isActive && !algos.empty()) {
			foreach (boost::shared_ptr<AlgoState> &algo, algos) {
				if (m_currentAlgos->find(algo) != m_currentAlgos->end()) {
					continue;
				}
				(*m_currentAlgos)[algo] = true;
			}
			m_positionsCheckCondition.notify_one();
		}
	}

	if (nextIterationTime <= boost::get_system_time()) {
		static volatile long reportsCount = 0;
		const auto currentReportCount = Interlocking::Increment(reportsCount);
		if (currentReportCount == 1 || currentReportCount > 50000) {
			Log::Warn("Dispatcher timeout thread is heavy loaded!");
			if (currentReportCount > 1) {
				Interlocking::Exchange(reportsCount, 0);
			}
		}
		return result;
	}

	boost::this_thread::sleep(nextIterationTime);

	return result;

}

bool Dispatcher::Notifier::AlgoIteration() {

	AlgoNotifyList *notifyList = nullptr;
	{
		Lock lock(m_mutex);
		if (m_isExit) {
			return false;
		}
		Assert(m_currentAlgos == &m_algoQueue.first || m_currentAlgos == &m_algoQueue.second);
		if (m_currentAlgos->empty()) {
			m_positionsCheckCondition.wait(lock);
			if (m_isExit) {
				return false;
			} else if (m_currentAlgos->empty() || !m_isActive) {
				return true;
			}
		} else {
			// Log::Warn("Dispatcher algos thread is heavy loaded!");
		}
		notifyList = m_currentAlgos;
		m_currentAlgos = m_currentAlgos == &m_algoQueue.first
			?	&m_algoQueue.second
			:	&m_algoQueue.first;
		if (m_settings->IsReplayMode()) {
			m_positionsCheckCompletedCondition.notify_all();
		}
	}

	Assert(!notifyList->empty());
	foreach (auto &notification, *notifyList) {
		notification.first->CheckPositions(notification.second);
	}
	notifyList->clear();

	return true;

}

bool Dispatcher::Notifier::ObserverIteration() {

	ObserverNotifyList *notifyList = nullptr;
	{

		Lock lock(m_mutex);
		if (m_isExit) {
			return false;
		}
		Assert(
			m_currentObservers == &m_observerQueue.first
			|| m_currentObservers == &m_observerQueue.second);
		if (m_currentObservers->empty()) {
			m_securityUpdateCondition.wait(lock);
			if (m_isExit) {
				return false;
			} else if (m_currentObservers->empty() || !m_isActive) {
				return true;
			}
		} else {
			// Log::Warn("Dispatcher algos thread is heavy loaded!");
		}
		notifyList = m_currentObservers;
		m_currentObservers = m_currentObservers == &m_observerQueue.first
			?	&m_observerQueue.second
			:	&m_observerQueue.first;
	}

	Assert(!notifyList->empty());
	foreach (auto &observationEvent, *notifyList) {
		observationEvent.state->NotifyUpdate(
			*observationEvent.security,
			observationEvent.price,
			observationEvent.qty,
			observationEvent.isBuy);
	}
	notifyList->clear();

	return true;

}

//////////////////////////////////////////////////////////////////////////

class Dispatcher::Slots : private boost::noncopyable {

public:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	typedef SignalConnectionList<Security::UpdateSlotConnection> DataUpdateConnections;

	Mutex m_dataUpdateMutex;
	DataUpdateConnections m_dataUpdateConnections;

};

class Dispatcher::FuSlots : private boost::noncopyable {

public:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	typedef SignalConnectionList<Security::FirstUpdateSlotConnection> DataUpdateConnections;

	Mutex m_dataUpdateMutex;
	DataUpdateConnections m_dataUpdateConnections;

};

////////////////////////////////////////////////////////////////////////////////

Dispatcher::Dispatcher(boost::shared_ptr<const Settings> options)
		: m_notifier(new Notifier(options)),
		m_slots(new Slots),
		m_fuSlots(new FuSlots) {
	//...//
}

Dispatcher::~Dispatcher() {
	//...//
}

void Dispatcher::Start() {
	m_notifier->Start();
}

void Dispatcher::Stop() {
	m_notifier->Stop();
}

void Dispatcher::Register(boost::shared_ptr<Algo> algo) {
	const Security &security = *const_cast<const Algo &>(*algo).GetSecurity();
	{
		const Notifier::Lock algoListlock(m_notifier->GetAlgoListMutex());
		Notifier::AlgoStateList &algoList = m_notifier->GetAlgoList();
		const Slots::Lock lock(m_slots->m_dataUpdateMutex);
		boost::shared_ptr<AlgoState> algoState(
			new AlgoState(algo, m_notifier, m_notifier->GetSettings()));
		algoList.push_back(algoState);
		m_slots->m_dataUpdateConnections.InsertSafe(
			security.Subcribe(
				Security::UpdateSlot(
					boost::bind(&Notifier::Signal, m_notifier.get(), algoState))));
		algoList.swap(m_notifier->GetAlgoList());
	}
	Log::Info(
		"Registered ALGO \"%1%\" (tag: \"%2%\") for security \"%3%\".",
		algo->GetName(),
		algo->GetTag(),
		security.GetFullSymbol());
}

void Dispatcher::Register(boost::shared_ptr<Observer> observer) {
	boost::shared_ptr<ObserverState> state(new ObserverState(observer));
	m_notifier->GetObserversStateList().push_back(state);
	const Slots::Lock lock(m_fuSlots->m_dataUpdateMutex);
	std::for_each(
		observer->GetNotifyList().begin(),
		observer->GetNotifyList().end(),
		[this, &state] (boost::shared_ptr<const Security> security) {
			m_fuSlots->m_dataUpdateConnections.InsertSafe(
				security->Subcribe(
					Security::FirstUpdateSlot(
						boost::bind(
							&Dispatcher::Notifier::Signal,
							m_notifier.get(),
							state,
							security,
							_1,
							_2,
							_3))));
		});
	Log::Info(
		"Registered OBSERVER \"%1%\" (tag: \"%2%\").",
		observer->GetName(),
		observer->GetTag());
}

void Dispatcher::CloseAll() {
	//...//
}

////////////////////////////////////////////////////////////////////////////////
