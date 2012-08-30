/**************************************************************************
 *   Created: 2012/07/22 23:41:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Util.hpp"
#include "Core/TradeSystem.hpp"
#include "Core/MarketDataSource.hpp"

namespace pt = boost::posix_time;
using namespace Trader;

void Connect(TradeSystem &tradeSystem, const Settings &settings) {
	for ( ; ; ) {
		try {
			tradeSystem.Connect(settings);
			break;
		} catch (const TradeSystem::ConnectError &) {
			boost::this_thread::sleep(pt::seconds(5));
		}
	}
}

void Connect(MarketDataSource &/*marketDataSource*/, const Settings &/*settings*/) {
	for ( ; ; ) {
		try {
			// marketDataSource.Connect(settings);
			break;
		} catch (const MarketDataSource::ConnectError &) {
			boost::this_thread::sleep(pt::seconds(5));
		}
	}
}
