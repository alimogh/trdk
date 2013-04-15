/**************************************************************************
 *   Created: 2012/09/19 23:58:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Observer.hpp"
#include "Service.hpp"

using namespace trdk;
using namespace trdk::Lib;

//////////////////////////////////////////////////////////////////////////

class Observer::Implementation : private boost::noncopyable {

public:

	NotifyList m_notifyList;

public:

	explicit Implementation(const Observer::NotifyList &notifyList)
			:  m_notifyList(notifyList) {
		//...//
	}

};

//////////////////////////////////////////////////////////////////////////

Observer::Observer(
			Context &context,
			const std::string &name,
			const std::string &tag,
			const Observer::NotifyList &notifyList)
		: Module(context, "Observer", name, tag),
		m_pimpl(new Implementation(notifyList)) {
	//...//
}

Observer::~Observer() {
	delete m_pimpl;
}

void Observer::OnLevel1Update(const trdk::Security &) {
	GetLog().Error(
		"Subscribed to Level 1 updates, but can't work with it"
			" (hasn't OnLevel1Update method implementation).");
	throw MethodDoesNotImplementedError(
		"Module subscribed to Level 1 updates, but can't work with it");
}

void Observer::OnNewTrade(
					const trdk::Security &,
					const boost::posix_time::ptime &,
					ScaledPrice,
					Qty,
					OrderSide) {
	GetLog().Error(
		"Subscribed to new trades, but can't work with it"
			" (hasn't OnNewTrade method implementation).");
	throw MethodDoesNotImplementedError(
		"Module subscribed to new trades, but can't work with it");
}

void Observer::OnServiceDataUpdate(const trdk::Service &service) {
	GetLog().Error(
		"Subscribed to \"%1%\", but can't work with it"
			" (hasn't OnServiceDataUpdate method implementation).",
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
			trdk::ScaledPrice price,
			trdk::Qty qty,
			trdk::OrderSide side) {
	const Lock lock(GetMutex());
	OnNewTrade(security, time, price, qty, side);
}

void Observer::RaiseServiceDataUpdateEvent(const Service &service) {
	const Lock lock(GetMutex());
	OnServiceDataUpdate(service);
}

const Observer::NotifyList & Observer::GetNotifyList() const {
	return m_pimpl->m_notifyList;
}
