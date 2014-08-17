/**************************************************************************
 *   Created: 2012/10/27 15:40:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "CsvSecurity.hpp"

using namespace trdk;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Csv;

Csv::Security::Security(
			Context &context,
			const Lib::Symbol &symbol,
			const MarketDataSource &source)
		: Base(context, symbol, source) {
	//...//
}

void Csv::Security::AddTrade(
			const boost::posix_time::ptime &time,
			OrderSide side,
			ScaledPrice price,
			trdk::Qty qty) {
	Base::AddTrade(time, side, price, qty, true, true);
}
