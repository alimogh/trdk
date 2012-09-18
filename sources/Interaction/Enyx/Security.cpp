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

void Security::OnSellOrderAdd(OrderId, Qty, double /*price*/) {
	//...//
}

void Security::OnBuyOrderAdd(OrderId, Qty, double /*price*/) {
	//...//
}

void Security::OnSellOrderExec(OrderId, Qty, double /*price*/) {
	//...//
}

void Security::OnBuyOrderExec(OrderId, Qty, double /*price*/) {
	//...//
}

void Security::OnBuyOrderChange(OrderId, Qty /*newQty*/, double /*newPrice*/) {
	//...//
}

void Security::OnSellOrderChange(OrderId, Qty /*newQty*/, double /*newPrice*/) {
	//...//
}

void Security::OnSellOrderDel(OrderId, Qty, double /*price*/) {
	//...//
}

void Security::OnBuyOrderDel(OrderId, Qty, double /*price*/) {
	//...//
}
