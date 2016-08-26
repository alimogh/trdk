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
#include "FakeTradingSystem.hpp"
#include "FakeMarketDataSource.hpp"
#include "Api.h"

TRDK_INTERACTION_FAKE_API
boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
		const trdk::TradingMode &mode,
		size_t index,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	const auto &result
		= boost::make_shared<trdk::Interaction::Fake::TradingSystem>(
			mode,
			index,
			context,
			tag,
			configuration);
	return result;
}

TRDK_INTERACTION_FAKE_API
boost::shared_ptr<trdk::MarketDataSource> CreateMarketDataSource(
		size_t index,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	return boost::shared_ptr<trdk::MarketDataSource>(
		new trdk::Interaction::Fake::MarketDataSource(
			index,
			context,
			tag,
			configuration));
}
