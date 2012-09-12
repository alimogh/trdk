/**************************************************************************
 *   Created: 2012/09/11 01:34:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#ifdef BOOST_WINDOWS
#	include "FakeMarketData.hpp"
#else
#	include "MarketData.hpp"
#endif

using namespace Trader;
using namespace Trader::Interaction::Enyx;

#ifdef BOOST_WINDOWS
	boost::shared_ptr< ::LiveMarketDataSource> CreateEnyxMarketDataSource() {
		return boost::shared_ptr< ::LiveMarketDataSource>(new FakeMarketData);
	}
#else
	extern "C" boost::shared_ptr< ::LiveMarketDataSource> CreateEnyxMarketDataSource() {
		static volatile long isEnyxInited = false;
		if (!Interlocking::Exchange(isEnyxInited, true)) {
			const auto result = EnyxMD::init();
			Log::Info(
				TRADER_ENYX_LOG_PREFFIX "%1% Market Data interfaces available on this system.",
				result);
		}
		return boost::shared_ptr< ::LiveMarketDataSource>(new MarketData);
	}
#endif
