/**************************************************************************
 *   Created: 2012/09/15 23:34:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "TradeSystem.hpp"
#include "MarketDataSource.hpp"

#ifdef BOOST_WINDOWS
#	define TRADER_INTERACTION_FAKE_API
#else
#	define TRADER_INTERACTION_FAKE_API extern "C"
#endif

TRADER_INTERACTION_FAKE_API
boost::shared_ptr<Trader::TradeSystem> CreateTradeSystem(
			const Trader::Lib::IniFileSectionRef &configuration,
			Trader::Context::Log &log) {
	return boost::shared_ptr<Trader::TradeSystem>(
		new Trader::Interaction::Fake::TradeSystem(configuration, log));
}

TRADER_INTERACTION_FAKE_API
boost::shared_ptr<Trader::MarketDataSource> CreateMarketDataSource(
			const Trader::Lib::IniFileSectionRef &configuration,
			Trader::Context::Log &log) {
	return boost::shared_ptr<Trader::MarketDataSource>(
		new Trader::Interaction::Fake::MarketDataSource(configuration, log));
}
