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

namespace {

	template<typename T>
	void ConnectUntilSuccess(T &obj, const IniFileSectionRef &settings) {
		for ( ; ; ) {
			try {
				obj.Connect(settings);
				break;
			} catch (const T::ConnectError &) {
				boost::this_thread::sleep(pt::seconds(5));
			}
		}
	}

}

void Engine::Connect(TradeSystem &ts, const IniFileSectionRef &settings) {
	ConnectUntilSuccess(ts, settings);
}

void Engine::Connect(MarketDataSource &md, const IniFileSectionRef &settings) {
	ConnectUntilSuccess(md, settings);
}
