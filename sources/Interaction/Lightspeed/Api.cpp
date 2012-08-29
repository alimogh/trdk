/**************************************************************************
 *   Created: 2012/08/28 01:40:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Gateway.hpp"
#include "MarketData.hpp"

namespace Trader {  namespace Interaction { namespace Lightspeed {

	boost::shared_ptr<TradeSystem> CreateTradeSystem() {
		return boost::shared_ptr<TradeSystem>(new Gateway);
	}

	boost::shared_ptr<::LiveMarketDataSource> CreateLiveMarketDataSource() {
		return boost::shared_ptr<::LiveMarketDataSource>(new MarketData);
	}

} } }