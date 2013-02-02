/**************************************************************************
 *   Created: 2012/10/27 15:40:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "CsvSecurity.hpp"

using namespace Trader;
using namespace Trader::Interaction;
using namespace Trader::Interaction::Csv;

Csv::Security::Security(
			Context &context,
			const std::string &symbol,
			const std::string &primaryExchange,
			const std::string &exchange,
			bool logMarketData)
		: Base(context, symbol, primaryExchange, exchange, logMarketData) {
	//...//
}

void Csv::Security::AddTrade(
			const boost::posix_time::ptime &time,
			OrderSide side,
			ScaledPrice price,
			Trader::Qty qty) {
	Base::AddTrade(time, side, price, qty, true);
}
