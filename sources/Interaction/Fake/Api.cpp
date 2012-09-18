/**************************************************************************
 *   Created: 2012/09/15 23:34:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "TradeSystem.hpp"
#include "LiveMarketDataSource.hpp"

#ifdef BOOST_WINDOWS
#	define TRADER_INTERACTION_FAKE_API
#else
#	define TRADER_INTERACTION_FAKE_API extern "C"
#endif

TRADER_INTERACTION_FAKE_API boost::shared_ptr<Trader::TradeSystem> CreateTradeSystem() {
	return boost::shared_ptr<Trader::TradeSystem>(
		new Trader::Interaction::Fake::TradeSystem);
}

TRADER_INTERACTION_FAKE_API boost::shared_ptr<Trader::LiveMarketDataSource> CreateLiveMarketDataSource() {
	return boost::shared_ptr<Trader::LiveMarketDataSource>(
		new Trader::Interaction::Fake::LiveMarketDataSource);
}
