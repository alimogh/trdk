/**************************************************************************
 *   Created: 2012/07/09 16:10:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Notifier.hpp"
#include "Dispatcher.hpp"
#include "Util.hpp"
#include "Core/Strategy.hpp"
#include "Core/Observer.hpp"

namespace mi = boost::multi_index;
namespace pt = boost::posix_time;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Engine;

//////////////////////////////////////////////////////////////////////////

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
			strategy.GetSecurity()->SubcribeToLevel1(
				Security::Level1UpdateSlot(
					boost::bind(
						&Notifier::Signal,
						m_notifier.get(),
						strategyState))));
		strategyList.swap(m_notifier->GetStrategyList());
	}
	Report(strategy, *strategy.GetSecurity(), "Level 1");
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
					&Notifier::Signal,
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
	boost::shared_ptr<TradeObserverState> state;
	{
		const Notifier::Lock strategyListlock(
			m_notifier->GetStrategyListMutex());
		Notifier::StrategyStateList strategyList(
			m_notifier->GetStrategyList());
		boost::shared_ptr<StrategyState> strategyState(
			new StrategyState(
				observer,
				m_notifier,
				m_notifier->GetSettings()));
		strategyList.push_back(strategyState);
		state.reset(new TradeObserverState(strategyState));
		m_notifier->GetTradeObserverStateList().push_back(state);
		strategyList.swap(m_notifier->GetStrategyList());
	}
	const TradesObservationSlots::Lock lock(
		m_tradesObservationSlots->m_dataUpdateMutex);
	SubscribeToNewTrades(state, *observer.GetSecurity());
}

void Dispatcher::SubscribeToNewTrades(Service &observer) {
	boost::shared_ptr<TradeObserverState> state(
		new TradeObserverState(observer.shared_from_this()));
	m_notifier->GetTradeObserverStateList().push_back(state);
	const TradesObservationSlots::Lock lock(
		m_tradesObservationSlots->m_dataUpdateMutex);
	SubscribeToNewTrades(state, *observer.GetSecurity());
}

void Dispatcher::SubscribeToNewTrades(Observer &observer) {
	boost::shared_ptr<TradeObserverState> state(
		new TradeObserverState(observer.shared_from_this()));
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
