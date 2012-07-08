/**************************************************************************
 *   Created: 2012/07/08 14:06:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Strategies/QuickArbitrage/QuickArbitrage.hpp"
#include "IqFeed/IqFeedClient.hpp"
#include "InteractiveBrokers/InteractiveBrokersTradeSystem.hpp"
#include "Dispatcher.hpp"
#include "Core/Security.hpp"

namespace pt = boost::posix_time;

namespace {

	void Connect(TradeSystem &tradeSystem) {
		for ( ; ; ) {
			try {
				tradeSystem.Connect();
				break;
			} catch (const TradeSystem::ConnectError &) {
				boost::this_thread::sleep(pt::seconds(5));
			}
		}
	}

	void Connect(IqFeedClient &marketDataSource) {
		for ( ; ; ) {
			try {
				marketDataSource.Connect();
				break;
			} catch (const IqFeedClient::ConnectError &) {
				boost::this_thread::sleep(pt::seconds(5));
			}
		}
	}

}

void Trade() {

	boost::shared_ptr<DynamicSecurity> fb(new DynamicSecurity("FB", "NASDAQ", "SMART", true));
	boost::shared_ptr<Strategies::QuickArbitrage::Algo> quickArbitrage(
		new Strategies::QuickArbitrage::Algo(fb));

	Dispatcher dispatcher;
	dispatcher.Register(quickArbitrage);

	boost::shared_ptr<TradeSystem> tradeSystem(new InteractiveBrokersTradeSystem);
	Connect(*tradeSystem);
	
	IqFeedClient marketDataSource;
	Connect(marketDataSource);
	
	marketDataSource.RequestHistory(fb, pt::time_from_string("2012-07-06 13:00:00"));
	marketDataSource.SubscribeToMarketData(fb);

	dispatcher.Start();
	getchar();

}


