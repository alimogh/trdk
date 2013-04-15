/**************************************************************************
 *   Created: 2012/09/15 23:34:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
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
boost::shared_ptr<trdk::TradeSystem> CreateTradeSystem(
			const trdk::Lib::IniFileSectionRef &configuration,
			trdk::Context::Log &log) {
	return boost::shared_ptr<trdk::TradeSystem>(
		new trdk::Interaction::Fake::TradeSystem(configuration, log));
}

TRADER_INTERACTION_FAKE_API
boost::shared_ptr<trdk::MarketDataSource> CreateMarketDataSource(
			const trdk::Lib::IniFileSectionRef &configuration,
			trdk::Context::Log &log) {
	return boost::shared_ptr<trdk::MarketDataSource>(
		new trdk::Interaction::Fake::MarketDataSource(configuration, log));
}
