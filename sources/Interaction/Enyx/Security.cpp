/**************************************************************************
 *   Created: 2012/09/16 17:08:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"

using namespace Trader::Interaction::Enyx;

Security::Security(
			boost::shared_ptr<Trader::TradeSystem> ts,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Trader::Settings> settings,
			bool logMarketData)
		: Base(ts, symbol, primaryExchange, exchange, settings, logMarketData) {
	//...//
}

Security::Security(
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			boost::shared_ptr<const Trader::Settings> settings,
			bool logMarketData)
		: Base(symbol, primaryExchange, exchange, settings, logMarketData) {
	//...//
}

void Security::SetAsk(
			const boost::posix_time::ptime &time,
			ScaledPrice price,
			Qty qty) {
	SetLastMarketDataTime(time);
	if (Base::SetAsk(price, qty)) {
		return;
	}
	SignalUpdate();
}

void Security::SetLastAndAsk(
			const boost::posix_time::ptime &time,
			ScaledPrice lastPrice,
			Qty lastQty,
			ScaledPrice askPrice,
			Qty askQty) {
	SetLastMarketDataTime(time);
	if (!Base::SetAsk(askPrice, askQty) && !Base::SetLast(lastPrice, lastQty)) {
		return;
	}
	SignalUpdate();
}

void Security::SetBid(
			const boost::posix_time::ptime &time,
			ScaledPrice price,
			Qty qty) {
	SetLastMarketDataTime(time);
	if (!Base::SetBid(price, qty)) {
		return;
	}
	SignalUpdate();
}

void Security::SetLastAndBid(
			const boost::posix_time::ptime &time,
			ScaledPrice lastPrice,
			Qty lastQty,
			ScaledPrice bidPrice,
			Qty bidQty) {
	SetLastMarketDataTime(time);
	if (!Base::SetBid(bidPrice, bidQty) && !Base::SetLast(lastPrice, lastQty)) {
		return;
	}
	SignalUpdate();
}

void Security::SetLast(
			const boost::posix_time::ptime &time,
			ScaledPrice price,
			Qty qty) {
	SetLastMarketDataTime(time);
	if (!Base::SetLast(price, qty)) {
		return;
	}
	SignalUpdate();
}

void Security::SetFirstUpdate(bool isBuy, ScaledPrice price, Qty qty) {
	Base::SetFirstUpdate(isBuy, price, qty);
}
