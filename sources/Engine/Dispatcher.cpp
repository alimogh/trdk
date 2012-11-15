/**************************************************************************
 *   Created: 2012/07/09 16:10:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Dispatcher.hpp"
#include "Util.hpp"
#include "Core/Strategy.hpp"
#include "Core/Observer.hpp"
#include "Core/Service.hpp"
#include "Core/Security.hpp"
#include "Core/PositionBundle.hpp"
#include "Core/Position.hpp"
#include "Core/PositionReporter.hpp"
#include "Core/Settings.hpp"

namespace mi = boost::multi_index;
namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Engine;

//////////////////////////////////////////////////////////////////////////

class Dispatcher::StrategyState
		: private boost::noncopyable,
		public boost::enable_shared_from_this<StrategyState> {

private:

	typedef SignalConnectionList<Position::StateUpdateConnection> StateUpdateConnections;

public:

	explicit StrategyState(
				Strategy &startegy,
				boost::shared_ptr<Notifier> notifier,
				boost::shared_ptr<const Settings> settings)
			: m_strategy(startegy.shared_from_this()),
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
		const Strategy::Lock lock(m_strategy->GetMutex());
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
		return m_isBlocked || !m_strategy->IsValidPrice(*m_settings);
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
				m_strategy->GetPositionReporter().ReportClosedPositon(*p);
				p->MarkAsReported();
			}
		}
	}

private:

	boost::shared_ptr<Strategy> m_strategy;
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

	explicit ObserverState(Observer &observer)
			: m_observer(observer.shared_from_this()) {
		//...//
	}

	void NotifyNewTrade(
				const Security &security,
				const boost::posix_time::ptime &time,
				Trader::Security::ScaledPrice price,
				Trader::Security::Qty qty,
				bool isBuy) {
		m_observer->OnNewTrade(security, time, price, qty, isBuy);
	}

private:

	boost::shared_ptr<Observer> m_observer;

};

//////////////////////////////////////////////////////////////////////////

class Dispatcher::Notifier : private boost::noncopyable {

public:

	typedef std::list<boost::shared_ptr<StrategyState>> StrategyStateList;
	typedef std::list<boost::shared_ptr<ObserverState>> ObserversStateList;
	
	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

private:

	typedef boost::condition_variable Condition;

	typedef std::map<
			boost::shared_ptr<StrategyState>,
			bool>
		StrategyNotifyList;

	struct ObservationEvent {
		boost::shared_ptr<ObserverState> state;
		boost::shared_ptr<const Security> security;
		boost::posix_time::ptime time;
		Security::ScaledPrice price;
		Security::Qty qty;
		bool isBuy;
	};
	typedef std::list<ObservationEvent> ObserverNotifyList;

public:

	explicit Notifier(boost::shared_ptr<const Settings> settings)
			: m_isActive(false),
			m_isExit(false),
			m_currentStrategies(&m_strategyQueue.first),
			m_heavyLoadedUpdatesCount(0),
			m_currentObservers(&m_observerQueue.first),
			m_heavyLoadedObservationsCount(0),
			m_settings(settings) {
		
		{
			const char *const threadName = "Strategy";
			Log::Info(
				"Starting %1% thread(s) \"%2%\"...",
				m_settings->GetThreadsCount(),
				threadName);
			boost::barrier startBarrier(m_settings->GetThreadsCount() + 1);
			for (size_t i = 0; i < m_settings->GetThreadsCount(); ++i) {
				m_threads.create_thread(
					[&]() {
						Task(
							startBarrier,
							threadName,
							&Dispatcher::Notifier::NotifyUpdate);
					});
			}
			startBarrier.wait();
			Log::Info("All \"%1%\" threads started.", threadName);
		}
		{
			boost::barrier startBarrier(1 + 1);
			m_threads.create_thread(
				[&]() {
					Task(
						startBarrier,
						"Observer",
						&Dispatcher::Notifier::NotifyNewObservationData);
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
						Task(
							startBarrier,
							threadName,
							&Dispatcher::Notifier::NotifyTimeout);
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

	Mutex & GetStrategyListMutex() {
		return m_strategyListMutex;
	}

	StrategyStateList & GetStrategyList() {
		return m_strategyList;
	}

	ObserversStateList & GetObserversStateList() {
		return m_observerList;
	}

	void Signal(boost::shared_ptr<StrategyState> strategyState) {
		
		if (strategyState->IsBlocked()) {
			return;
		}
		
		Lock lock(m_mutex);
		if (m_isExit) {
			return;
		}

		const StrategyNotifyList::const_iterator i = m_currentStrategies->find(strategyState);
		if (i != m_currentStrategies->end() && !i->second) {
			return;
		}
		(*m_currentStrategies)[strategyState] = false;
		m_positionsCheckCondition.notify_one();
		if (m_settings->IsReplayMode()) {
			m_positionsCheckCompletedCondition.wait(lock);
		}

	}

	void Signal(
				boost::shared_ptr<ObserverState> observerState,
				const boost::shared_ptr<const Security> &security,
				const boost::posix_time::ptime &time,
				Security::ScaledPrice price,
				Security::Qty qty,
				bool isBuy) {

		ObservationEvent observationEvent = {};
		observationEvent.state = observerState;
		observationEvent.security = security;
		observationEvent.time = time;
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

	bool NotifyTimeout();
	bool NotifyUpdate();
	bool NotifyNewObservationData();

private:

	bool m_isActive;
	bool m_isExit;

	Mutex m_mutex;
	Condition m_positionsCheckCondition;
	Condition m_positionsCheckCompletedCondition;

	Condition m_securityUpdateCondition;

	std::pair<StrategyNotifyList, StrategyNotifyList> m_strategyQueue;
	StrategyNotifyList *m_currentStrategies;
	size_t m_heavyLoadedUpdatesCount;

	std::pair<ObserverNotifyList, ObserverNotifyList> m_observerQueue;
	ObserverNotifyList *m_currentObservers;
	size_t m_heavyLoadedObservationsCount;

	boost::shared_ptr<const Settings> m_settings;

	Mutex m_strategyListMutex;
	StrategyStateList m_strategyList;
	
	ObserversStateList m_observerList;

	boost::thread_group m_threads;

};

//////////////////////////////////////////////////////////////////////////

bool Dispatcher::StrategyState::CheckPositionsUnsafe() {

	const auto now = boost::get_system_time();
	Interlocking::Exchange(
		m_lastUpdate,
		(now - m_settings->GetStartTime()).total_milliseconds());

	Assert(!m_isBlocked);
		
	const Security &security = *const_cast<const Strategy &>(*m_strategy).GetSecurity();
	Assert(security || !m_settings->ShouldWaitForMarketData()); // must be checked it security object

	if (m_positions) {
		Assert(!m_positions->Get().empty());
		Assert(m_stateUpdateConnections.IsConnected());
		ReportClosedPositon(*m_positions);
		if (!m_positions->IsCompleted()) {
			if (m_positions->IsOk()) {
				if (	!m_settings->IsReplayMode()
						&& m_settings->GetCurrentTradeSessionEndime() <= now) {
					foreach (auto &p, m_positions->Get()) {
						if (p->IsCanceled()) {
							continue;
						}
						p->CancelAtMarketPrice(
							Position::CLOSE_TYPE_SCHEDULE);
					}
				} else {
					m_strategy->TryToClosePositions(*m_positions);
				}
			} else {
				Log::Warn(
					"Strategy \"%1%\" BLOCKED by dispatcher.",
					m_strategy->GetName());
				Interlocking::Exchange(m_isBlocked, true);
			}
			return false;
		}
		m_stateUpdateConnections.Diconnect();
		m_positions.reset();
	}

	Assert(!m_stateUpdateConnections.IsConnected());

	boost::shared_ptr<PositionBandle> positions = m_strategy->TryToOpenPositions();
	if (!positions || positions->Get().empty()) {
		return false;
	}

	StateUpdateConnections stateUpdateConnections;
	foreach (const auto &p, positions->Get()) {
		Assert(&p->GetSecurity() == &security);
		m_strategy->ReportDecision(*p);
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

bool Dispatcher::Notifier::NotifyTimeout() {

	const auto nextIterationTime
		= boost::get_system_time() + pt::milliseconds(m_settings->GetUpdatePeriodMilliseconds());

	std::list<boost::shared_ptr<StrategyState>> strategies;
	{
		const Lock lock(m_strategyListMutex);
		foreach (boost::shared_ptr<StrategyState> &strategy, m_strategyList) {
			if (strategy->IsTimeToUpdate()) {
				strategies.push_back(strategy);
			}
		}
	}
	bool result = true;
	{
		Lock lock(m_mutex);
		if (m_isExit) {
			result = false;
		} else if (m_isActive && !strategies.empty()) {
			foreach (
					boost::shared_ptr<StrategyState> &strategy,
					strategies) {
				if (	m_currentStrategies->find(strategy)
							!= m_currentStrategies->end()) {
					continue;
				}
				(*m_currentStrategies)[strategy] = true;
			}
			m_positionsCheckCondition.notify_one();
		}
	}

	if (nextIterationTime <= boost::get_system_time()) {
		static volatile long reportsCount = 0;
		const auto currentReportCount
			= Interlocking::Increment(reportsCount);
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

bool Dispatcher::Notifier::NotifyUpdate() {

	StrategyNotifyList *notifyList = nullptr;
	{
		Lock lock(m_mutex);
		if (m_isExit) {
			return false;
		}
		Assert(
			m_currentStrategies == &m_strategyQueue.first
			|| m_currentStrategies == &m_strategyQueue.second);
		if (m_currentStrategies->empty()) {
			m_heavyLoadedUpdatesCount = 0;
			m_positionsCheckCondition.wait(lock);
			if (m_isExit) {
				return false;
			} else if (m_currentStrategies->empty() || !m_isActive) {
				return true;
			}
		} else if (!(++m_heavyLoadedUpdatesCount % 100)) {
			Log::Warn(
				"Dispatcher updates thread is heavy loaded (%1% iterations)!",
				m_heavyLoadedUpdatesCount);
		}
		notifyList = m_currentStrategies;
		m_currentStrategies = m_currentStrategies == &m_strategyQueue.first
			?	&m_strategyQueue.second
			:	&m_strategyQueue.first;
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

bool Dispatcher::Notifier::NotifyNewObservationData() {

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
			m_heavyLoadedObservationsCount = 0;
			m_securityUpdateCondition.wait(lock);
			if (m_isExit) {
				return false;
			} else if (m_currentObservers->empty() || !m_isActive) {
				return true;
			}
		} else if (!(++m_heavyLoadedUpdatesCount % 100)) {
			Log::Warn(
				"Dispatcher observers thread is heavy loaded (%1% iterations)!",
				m_heavyLoadedUpdatesCount);
		}
		notifyList = m_currentObservers;
		m_currentObservers = m_currentObservers == &m_observerQueue.first
			?	&m_observerQueue.second
			:	&m_observerQueue.first;
	}

	Assert(!notifyList->empty());
	foreach (auto &observationEvent, *notifyList) {
		observationEvent.state->NotifyNewTrade(
			*observationEvent.security,
			observationEvent.time,
			observationEvent.price,
			observationEvent.qty,
			observationEvent.isBuy);
	}
	notifyList->clear();

	return true;

}

//////////////////////////////////////////////////////////////////////////

class Dispatcher::UpdateSlots : private boost::noncopyable {

public:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	typedef SignalConnectionList<Security::Level1UpdateSlotConnection> DataUpdateConnections;

	Mutex m_dataUpdateMutex;
	DataUpdateConnections m_dataUpdateConnections;

};

class Dispatcher::ObservationSlots : private boost::noncopyable {

public:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	typedef SignalConnectionList<Security::NewTradeSlotConnection> DataUpdateConnections;

	Mutex m_dataUpdateMutex;
	DataUpdateConnections m_dataUpdateConnections;

};

namespace {

	template<typename Module>
	void Report(
				const Module &module,
				const Security &security,
				const char *type) {
		Log::Info(
			"%1% \"%2%\" (tag: \"%3%\") subscribed to \"%4%\" %5%.",
			ModuleNamePolicy<Module>::GetNameFirstCapital(),
			module.GetName(),
			module.GetTag(),
			security.GetFullSymbol(),
			type);
	}

}

////////////////////////////////////////////////////////////////////////////////

Dispatcher::Dispatcher(boost::shared_ptr<const Settings> options)
		: m_notifier(new Notifier(options)),
		m_updateSlots(new UpdateSlots),
		m_observationSlots(new ObservationSlots) {
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

void Dispatcher::SubscribeToLevel1(Strategy &strategy) {
	const Security &security
		= *const_cast<const Strategy &>(strategy).GetSecurity();
	{
		const Notifier::Lock strategyListlock(
			m_notifier->GetStrategyListMutex());
		Notifier::StrategyStateList &strategyList
			= m_notifier->GetStrategyList();
		const UpdateSlots::Lock lock(m_updateSlots->m_dataUpdateMutex);
		boost::shared_ptr<StrategyState> strategyState(
			new StrategyState(
				strategy,
				m_notifier,
				m_notifier->GetSettings()));
		strategyList.push_back(strategyState);
		m_updateSlots->m_dataUpdateConnections.InsertSafe(
			security.SubcribeToLevel1(
				Security::Level1UpdateSlot(
					boost::bind(
						&Notifier::Signal,
						m_notifier.get(),
						strategyState))));
		strategyList.swap(m_notifier->GetStrategyList());
	}
	Report(strategy, security, "Level 1");
}

void Dispatcher::SubscribeToLevel1(Service &service) {
	const Security &security
		= *const_cast<const Service &>(service).GetSecurity();
	{

	}
	Report(service, security, "Level 1");
}

void Dispatcher::SubscribeToLevel1(Observer &) {
	throw Exception(
		"Level 1 updates for observers doesn't implemented yet");
}

void Dispatcher::SubscribeToNewTrades(Strategy &) {
	throw Exception(
		"New trades observation for strategies doesn't implemented yet");
}

void Dispatcher::SubscribeToNewTrades(Service &) {
	throw Exception(
		"New trades observation for services doesn't implemented yet");
}

void Dispatcher::SubscribeToNewTrades(Observer &observer) {
	boost::shared_ptr<ObserverState> state(new ObserverState(observer));
	m_notifier->GetObserversStateList().push_back(state);
	const UpdateSlots::Lock lock(m_observationSlots->m_dataUpdateMutex);
	std::for_each(
		observer.GetNotifyList().begin(),
		observer.GetNotifyList().end(),
		[&] (boost::shared_ptr<const Security> security) {
			m_observationSlots->m_dataUpdateConnections.InsertSafe(
				security->SubcribeToTrades(
					Security::NewTradeSlot(
						boost::bind(
							&Dispatcher::Notifier::Signal,
							m_notifier.get(),
							state,
							security,
							_1,
							_2,
							_3,
							_4))));
				Report(observer, *security, "Level 1");
		});
}

////////////////////////////////////////////////////////////////////////////////
