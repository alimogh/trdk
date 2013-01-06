/**************************************************************************
 *   Created: 2012/09/19 23:58:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Observer.hpp"
#include "Service.hpp"

using namespace Trader;
using namespace Trader::Lib;

//////////////////////////////////////////////////////////////////////////

namespace {
	const std::string typeName = "Observer";
}

//////////////////////////////////////////////////////////////////////////

class Observer::Implementation {

public:

	NotifyList m_notifyList;
	boost::shared_ptr<Trader::TradeSystem> tradeSystem;

public:

	explicit Implementation(
				const Observer::NotifyList &notifyList,
				boost::shared_ptr<Trader::TradeSystem> tradeSystem)
			: m_notifyList(notifyList),
			tradeSystem(tradeSystem) {
		//...//
	}

	~Implementation() {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

Observer::Observer(
			const std::string &tag,
			const Observer::NotifyList &notifyList,
			boost::shared_ptr<Trader::TradeSystem> tradeSystem,
			boost::shared_ptr<const Settings> settings)
		: Module(tag, settings),
		m_pimpl(new Implementation(notifyList, tradeSystem)) {
	//...//
}

Observer::~Observer() {
	delete m_pimpl;
}

void Observer::OnLevel1Update(const Trader::Security &) {
	Log::Error(
		"\"%1%\" subscribed to Level 1 updates, but can't work with it"
			" (hasn't implementation of OnLevel1Update).",
		*this);
	throw MethodDoesNotImplementedError(
		"Module subscribed to Level 1 updates, but can't work with it");
}

void Observer::OnNewTrade(
					const Trader::Security &,
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide) {
	Log::Error(
		"\"%1%\" subscribed to new trades, but can't work with it"
			" (hasn't implementation of OnNewTrade).",
		*this);
	throw MethodDoesNotImplementedError(
		"Module subscribed to new trades, but can't work with it");
}

void Observer::OnServiceDataUpdate(const Trader::Service &service) {
	Log::Error(
		"\"%1%\" subscribed to \"%2%\", but can't work with it"
			" (hasn't implementation of OnServiceDataUpdate).",
		*this,
		service);
 	throw MethodDoesNotImplementedError(
 		"Module subscribed to service, but can't work with it");
}

void Observer::RaiseLevel1UpdateEvent(const Security &security) {
	const Lock lock(GetMutex());
	OnLevel1Update(security);
}

void Observer::RaiseNewTradeEvent(
			const Security &security,
			const boost::posix_time::ptime &time,
			Trader::ScaledPrice price,
			Trader::Qty qty,
			Trader::OrderSide side) {
	const Lock lock(GetMutex());
	OnNewTrade(security, time, price, qty, side);
}

void Observer::RaiseServiceDataUpdateEvent(const Service &service) {
	const Lock lock(GetMutex());
	OnServiceDataUpdate(service);
}

const std::string & Observer::GetTypeName() const {
	return typeName;
}

const Observer::NotifyList & Observer::GetNotifyList() const {
	return m_pimpl->m_notifyList;
}

TradeSystem & Observer::GetTradeSystem() {
	return *m_pimpl->tradeSystem;
}
