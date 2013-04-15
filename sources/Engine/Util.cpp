/**************************************************************************
 *   Created: 2012/07/22 23:41:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Util.hpp"
#include "Core/TradeSystem.hpp"
#include "Core/MarketDataSource.hpp"

namespace pt = boost::posix_time;
using namespace trdk;
using namespace trdk::Lib;

void Engine::Connect(
			TradeSystem &tradeSystem,
			const IniFile &ini,
			const std::string &section) {
	for ( ; ; ) {
		try {
			tradeSystem.Connect(ini, section);
			break;
		} catch (const TradeSystem::ConnectError &) {
			boost::this_thread::sleep(pt::seconds(5));
		}
	}
}

void Engine::Connect(MarketDataSource &marketDataSource) {
	for ( ; ; ) {
		try {
			marketDataSource.Connect();
			break;
		} catch (const MarketDataSource::ConnectError &) {
			boost::this_thread::sleep(pt::seconds(5));
		}
	}
}
