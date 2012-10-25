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
			Qty qty,
			size_t pos) {
	SetLastMarketDataTime(time);
	Base::SetAsk(price, qty, pos);
	SignalUpdate();
}

void Security::SetLastAndAsk(
			const boost::posix_time::ptime &time,
			ScaledPrice lastPrice,
			Qty lastQty,
			ScaledPrice askPrice,
			Qty askQty) {
	SetLastMarketDataTime(time);
	Base::SetAsk(askPrice, askQty, 1);
	Base::SetLast(lastPrice, lastQty);
	SignalUpdate();
}

void Security::SetBid(
			const boost::posix_time::ptime &time,
			ScaledPrice price,
			Qty qty,
			size_t pos) {
	SetLastMarketDataTime(time);
	Base::SetBid(price, qty, pos);
}

void Security::SetLastAndBid(
			const boost::posix_time::ptime &time,
			ScaledPrice lastPrice,
			Qty lastQty,
			ScaledPrice bidPrice,
			Qty bidQty) {
	SetLastMarketDataTime(time);
	Base::SetBid(bidPrice, bidQty, 1);
	Base::SetLast(lastPrice, lastQty);
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

void Security::SignalNewTrade(
			const boost::posix_time::ptime &time,
			bool isBuy,
			ScaledPrice price,
			Qty qty) {
	Base::SignalNewTrade(time, isBuy, price, qty);
}
