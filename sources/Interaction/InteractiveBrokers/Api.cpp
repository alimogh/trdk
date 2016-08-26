/**************************************************************************
 *   Created: 2013/04/28 22:54:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "IbTradingSystem.hpp"
#include "Api.h"

namespace ib = trdk::Interaction::InteractiveBrokers;

TRDK_INTERACTION_INTERACTIVEBROKERS_API
trdk::TradingSystemAndMarketDataSourceFactoryResult
CreateTradingSystemAndMarketDataSource(
		const trdk::TradingMode &mode,
		size_t tradingSystemIndex,
		size_t marketDataSourceIndex,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	const auto &object = boost::make_shared<ib::TradingSystem>(
			mode,
			tradingSystemIndex,
			marketDataSourceIndex,
			context,
			tag,
			configuration);
	const trdk::TradingSystemAndMarketDataSourceFactoryResult result = {
		object,
		object
	};
	return result;
}

TRDK_INTERACTION_INTERACTIVEBROKERS_API
boost::shared_ptr<trdk::TradingSystem>
CreateTradingSystem(
		const trdk::TradingMode &mode,
		size_t tradingSystemIndex,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	const auto &result = boost::make_shared<ib::TradingSystem>(
		mode,
		tradingSystemIndex,
		std::numeric_limits<size_t>::max(),
		context,
		tag,
		configuration);
	return result;
}
