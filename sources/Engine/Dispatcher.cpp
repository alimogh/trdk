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

namespace {

	//////////////////////////////////////////////////////////////////////////

	struct GetModuleVisitor : public boost::static_visitor<Module &> {
		template<typename ModulePtr>
		Module & operator ()(ModulePtr &module) const {
			return *module;
		}
	
	};

	//////////////////////////////////////////////////////////////////////////

	void NotifyServiceDataUpdateSubscribers(const Service &);

	class NotifyServiceDataUpdateVisitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:
		explicit NotifyServiceDataUpdateVisitor(const Service &service)
				: m_service(service) {
			//...//
		}
	public:		
		template<typename Module>
		void operator ()(const boost::shared_ptr<Module> &module) const {
			NotifyObserver(*module);
		}
		template<>
		void operator ()(const boost::shared_ptr<Service> &module) const {
			NotifyObserver(*module);
			NotifyServiceDataUpdateSubscribers(*module);
		}
	public:
		template<typename Module>
		void NotifyObserver(Module &module) const {
			const Module::Lock lock(module.GetMutex());
			module.OnServiceDataUpdate(m_service);
		}
	private:
		const Service &m_service;
	};

	void NotifyServiceDataUpdateSubscribers(const Service &service) {
		const NotifyServiceDataUpdateVisitor visitor(service);
		foreach (auto subscriber, service.GetSubscribers()) {
			try {
				boost::apply_visitor(visitor, subscriber);
			} catch (const MethodDoesNotImplementedError &ex) {
				Log::Error(
					"Error at service subscribers notification:"
						" \"%1%\" (service: \"%2%\", subscriber: \"%3%\").",
					ex,
					service,
					boost::apply_visitor(GetModuleVisitor(), subscriber));
				throw;
			} catch (const Exception &ex) {
				Log::Error(
					"Error at service subscribers notification:"
						" \"%1%\" (service: \"%2%\", subscriber: \"%3%\").",
					ex,
					service,
					boost::apply_visitor(GetModuleVisitor(), subscriber));
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

}


//////////////////////////////////////////////////////////////////////////

class Dispatcher::StrategyState
		: private boost::noncopyable,
		public boost::enable_shared_from_this<Dispatcher::StrategyState> {

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

class Dispatcher::TradeObserverState
		: private boost::noncopyable,
		public boost::enable_shared_from_this<TradeObserverState> {

public:

	struct Trade {
		boost::shared_ptr<TradeObserverState> state;
		boost::shared_ptr<const Security> security;
		boost::posix_time::ptime time;
		ScaledPrice price;
		Qty qty;
		OrderSide side;
	};

private:

	class NotifyVisitor
			: public boost::static_visitor<void>,
			private boost::noncopyable {
	public:		
		explicit NotifyVisitor(const Trade &trade)
				: m_trade(trade) {
			//...//
		}
		template<typename Module>
		void operator ()(const boost::shared_ptr<Module> &observer) const {
			NotifyObserver(*observer);
		}
		template<>
		void operator ()(const boost::shared_ptr<Service> &observer) const {
			NotifyObserver(*observer);
			NotifyServiceDataUpdateSubscribers(*observer);
		}
	private:
		void NotifyObserver(SecurityAlgo &observer) const {
			AssertEq(
				m_trade.security->GetFullSymbol(),
				const_cast<const SecurityAlgo &>(observer)
					.GetSecurity()
					->GetFullSymbol());
			const Module::Lock lock(observer.GetMutex());
			observer.OnNewTrade(
				m_trade.time,
				m_trade.price,
				m_trade.qty,
				m_trade.side);
		}
		void NotifyObserver(Observer &observer) const {
			const Module::Lock lock(observer.GetMutex());
			observer.OnNewTrade(
				*m_trade.security,
				m_trade.time,
				m_trade.price,
				m_trade.qty,
				m_trade.side);
		}
	private:
		const Trade &m_trade;
	};

public:

	template<typename Module>
	explicit TradeObserverState(Module &observer)
			: m_observer(observer.shared_from_this()) {
		//...//
	}

	void NotifyNewTrades(const Trade &trade) {
		boost::apply_visitor(NotifyVisitor(trade), m_observer);
	}

	Module & GetObserver() {
		return boost::apply_visitor(GetModuleVisitor(), m_observer);
	}

private:

	boost::variant<
			boost::shared_ptr<Strategy>,
			boost::shared_ptr<Service>,
			boost::shared_ptr<Observer>>
		m_observer;

};

//////////////////////////////////////////////////////////////////////////

class Dispatcher::Notifier : private boost::noncopyable {

public:

	typedef std::list<boost::shared_ptr<StrategyState>> StrategyStateList;
	typedef std::list<
			boost::shared_ptr<TradeObserverState>>
		TradeObserverStateList;
	
	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

private:

	typedef boost::condition_variable Condition;

	typedef std::map<
			boost::shared_ptr<StrategyState>,
			bool>
		Level1UpdateNotifyList;

	typedef std::list<TradeObserverState::Trade> TradeNotifyList;

public:

	explicit Notifier(boost::shared_ptr<const Settings> settings)
			: m_isActive(false),
			m_isExit(false),
			m_currentLevel1Updates(&m_level1UpdatesQueue.first),
			m_heavyLoadedUpdatesCount(0),
			m_currentTradeObservervation(&m_tradeObservervationQueue.first),
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
							&Dispatcher::Notifier::NotifyLevel1Update);
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
						&Dispatcher::Notifier::NotifyNewTrades);
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
				m_tradeReadCompletedCondition.notify_all();
				m_positionsCheckCondition.notify_all();
				m_newTradesCondition.notify_all();
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

	TradeObserverStateList & GetTradeObserverStateList() {
		return m_tradeObserverStateList;
	}

	void Signal(boost::shared_ptr<StrategyState> strategyState) {
		
		if (strategyState->IsBlocked()) {
			return;
		}
		
		Lock lock(m_mutex);
		if (m_isExit) {
			return;
		}

		const Level1UpdateNotifyList::const_iterator i
			= m_currentLevel1Updates->find(strategyState);
		if (i != m_currentLevel1Updates->end() && !i->second) {
			return;
		}
		(*m_currentLevel1Updates)[strategyState] = false;
		m_positionsCheckCondition.notify_one();
		if (m_settings->IsReplayMode()) {
			m_positionsCheckCompletedCondition.wait(lock);
		}

	}

	void Signal(
				boost::shared_ptr<TradeObserverState> state,
				const boost::shared_ptr<const Security> &security,
				const boost::posix_time::ptime &time,
				ScaledPrice price,
				Qty qty,
				OrderSide side) {
		const TradeObserverState::Trade trade = {
			state,
			security,
			time,
			price,
			qty,
			side};
		Lock lock(m_mutex);
		if (m_isExit) {
			return;
		}
		m_currentTradeObservervation->push_back(trade);
		m_newTradesCondition.notify_one();
		if (m_settings->IsReplayMode()) {
			m_tradeReadCompletedCondition.wait(lock);
		}
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
	bool NotifyLevel1Update();
	bool NotifyNewTrades();

private:

	bool m_isActive;
	bool m_isExit;

	Mutex m_mutex;
	Condition m_positionsCheckCondition;
	Condition m_positionsCheckCompletedCondition;
	Condition m_tradeReadCompletedCondition;

	Condition m_newTradesCondition;

	std::pair<
			Level1UpdateNotifyList,
			Level1UpdateNotifyList>
		m_level1UpdatesQueue;
	Level1UpdateNotifyList *m_currentLevel1Updates;
	size_t m_heavyLoadedUpdatesCount;

	std::pair<TradeNotifyList, TradeNotifyList> m_tradeObservervationQueue;
	TradeNotifyList *m_currentTradeObservervation;
	size_t m_heavyLoadedObservationsCount;

	boost::shared_ptr<const Settings> m_settings;

	Mutex m_strategyListMutex;
	StrategyStateList m_strategyList;
	
	TradeObserverStateList m_tradeObserverStateList;

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
				boost::bind(
					&Notifier::Signal,
					m_notifier.get(),
					shared_from_this())));
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
				if (	m_currentLevel1Updates->find(strategy)
							!= m_currentLevel1Updates->end()) {
					continue;
				}
				(*m_currentLevel1Updates)[strategy] = true;
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

bool Dispatcher::Notifier::NotifyLevel1Update() {

	Level1UpdateNotifyList *notifyList = nullptr;
	{
		Lock lock(m_mutex);
		if (m_isExit) {
			return false;
		}
		Assert(
			m_currentLevel1Updates == &m_level1UpdatesQueue.first
			|| m_currentLevel1Updates == &m_level1UpdatesQueue.second);
		if (m_currentLevel1Updates->empty()) {
			m_heavyLoadedUpdatesCount = 0;
			m_positionsCheckCondition.wait(lock);
			if (m_isExit) {
				return false;
			} else if (m_currentLevel1Updates->empty() || !m_isActive) {
				return true;
			}
		} else if (!(++m_heavyLoadedUpdatesCount % 100)) {
			Log::Warn(
				"Dispatcher updates thread is heavy loaded (%1% iterations)!",
				m_heavyLoadedUpdatesCount);
		}
		notifyList = m_currentLevel1Updates;
		m_currentLevel1Updates = m_currentLevel1Updates == &m_level1UpdatesQueue.first
			?	&m_level1UpdatesQueue.second
			:	&m_level1UpdatesQueue.first;
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

bool Dispatcher::Notifier::NotifyNewTrades() {

	TradeNotifyList *notifyList = nullptr;
	{

		Lock lock(m_mutex);
		if (m_isExit) {
			return false;
		}
		Assert(
			m_currentTradeObservervation == &m_tradeObservervationQueue.first
			|| m_currentTradeObservervation == &m_tradeObservervationQueue.second);
		if (m_currentTradeObservervation->empty()) {
			m_heavyLoadedObservationsCount = 0;
			m_newTradesCondition.wait(lock);
			if (m_isExit) {
				return false;
			} else if (m_currentTradeObservervation->empty() || !m_isActive) {
				return true;
			}
		} else if (!(++m_heavyLoadedUpdatesCount % 100)) {
			Log::Warn(
				"Dispatcher observers thread is heavy loaded (%1% iterations)!",
				m_heavyLoadedUpdatesCount);
		}
		notifyList = m_currentTradeObservervation;
		m_currentTradeObservervation = m_currentTradeObservervation == &m_tradeObservervationQueue.first
			?	&m_tradeObservervationQueue.second
			:	&m_tradeObservervationQueue.first;
		if (m_settings->IsReplayMode()) {
			m_tradeReadCompletedCondition.notify_all();
		}
	}

	Assert(!notifyList->empty());
	foreach (auto &observationEvent, *notifyList) {
		observationEvent.state->NotifyNewTrades(observationEvent);
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

class Dispatcher::TradesObservationSlots : private boost::noncopyable {

public:

	typedef boost::mutex Mutex;
	typedef Mutex::scoped_lock Lock;

	typedef SignalConnectionList<
			Security::NewTradeSlotConnection>
		DataUpdateConnections;

	Mutex m_dataUpdateMutex;
	DataUpdateConnections m_dataUpdateConnections;

};

namespace {

	void Report(
				const Module &module,
				const Security &security,
				const char *type) {
		Log::Info(
			"\"%1%\" subscribed to %2% from \"%3%\".",
			module,
			type,
			security);
	}

}

////////////////////////////////////////////////////////////////////////////////

Dispatcher::Dispatcher(boost::shared_ptr<const Settings> options)
		: m_notifier(new Notifier(options)),
		m_updateSlots(new UpdateSlots),
		m_tradesObservationSlots(new TradesObservationSlots) {
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

void Dispatcher::SubscribeToLevel1(Service &) {
	throw Exception(
		"Level 1 updates for service doesn't implemented yet");
}

void Dispatcher::SubscribeToLevel1(Observer &) {
	throw Exception(
		"Level 1 updates for observers doesn't implemented yet");
}

void Dispatcher::SubscribeToNewTrades(
			boost::shared_ptr<TradeObserverState> &state,
			const Security &security) {
	m_tradesObservationSlots->m_dataUpdateConnections.InsertSafe(
		security.SubcribeToTrades(
			Security::NewTradeSlot(
				boost::bind(
					&Dispatcher::Notifier::Signal,
					m_notifier.get(),
					state,
					security.shared_from_this(),
					_1,
					_2,
					_3,
					_4))));
	Report(state->GetObserver(), security, "new trades");
}

void Dispatcher::SubscribeToNewTrades(Strategy &observer) {
	const Security &security
		= *const_cast<const Strategy &>(observer).GetSecurity();
	boost::shared_ptr<TradeObserverState> state(
		new TradeObserverState(observer));
	m_notifier->GetTradeObserverStateList().push_back(state);
	const TradesObservationSlots::Lock lock(
		m_tradesObservationSlots->m_dataUpdateMutex);
	SubscribeToNewTrades(state, security);
}

void Dispatcher::SubscribeToNewTrades(Service &observer) {
	const Security &security
		= *const_cast<const Service &>(observer).GetSecurity();
	boost::shared_ptr<TradeObserverState> state(
		new TradeObserverState(observer));
	m_notifier->GetTradeObserverStateList().push_back(state);
	const TradesObservationSlots::Lock lock(
		m_tradesObservationSlots->m_dataUpdateMutex);
	SubscribeToNewTrades(state, security);
}

void Dispatcher::SubscribeToNewTrades(Observer &observer) {
	boost::shared_ptr<TradeObserverState> state(
		new TradeObserverState(observer));
	m_notifier->GetTradeObserverStateList().push_back(state);
	const TradesObservationSlots::Lock lock(
		m_tradesObservationSlots->m_dataUpdateMutex);
	std::for_each(
		observer.GetNotifyList().begin(),
		observer.GetNotifyList().end(),
		[&] (boost::shared_ptr<const Security> security) {
			SubscribeToNewTrades(state, *security);
		});
}

////////////////////////////////////////////////////////////////////////////////
