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

Observer::Observer(
			Context &context,
			const std::string &name,
			const std::string &tag)
		: Consumer(context, "Observer", name, tag) {
	//...//
}

Observer::~Observer() {
	//...//
}

void Observer::RaiseLevel1UpdateEvent(Security &security) {
	const Lock lock(GetMutex());
	OnLevel1Update(security);
}

void Observer::RaiseLevel1TickEvent(
			Security &security,
			const boost::posix_time::ptime &time,
			const Level1TickValue &value) {
	const Lock lock(GetMutex());
	OnLevel1Tick(security, time, value);
}

void Observer::RaiseNewTradeEvent(
			Security &security,
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
