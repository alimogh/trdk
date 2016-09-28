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
#include "TradingSystem.hpp"
#include "RandomMarketDataSource.hpp"
#include "Api.h"

TRDK_INTERACTION_TEST_API
boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
		const trdk::TradingMode &mode,
		size_t index,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	return boost::make_shared<trdk::Interaction::Test::TradingSystem>(
		mode,
		index,
		context,
		tag,
		configuration);
}

TRDK_INTERACTION_TEST_API
boost::shared_ptr<trdk::MarketDataSource> CreateRandomMarketDataSource(
		size_t index,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	return boost::make_shared<trdk::Interaction::Test::RandomMarketDataSource>(
		index,
		context,
		tag,
		configuration);
}
