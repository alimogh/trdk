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
#include "IbTradeSystem.hpp"
#include "Api.h"

namespace ib = trdk::Interaction::InteractiveBrokers;

TRDK_INTERACTION_INTERACTIVEBROKERS_API
trdk::TradeSystemFactoryResult CreateTradeSystem(
		const trdk::TradingMode &mode,
		size_t index,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	trdk::TradeSystemFactoryResult result;
	const auto &tradeSystem = boost::make_shared<ib::TradeSystem>(
		mode,
		index,
		context,
		tag,
		configuration);
	boost::get<0>(result) = tradeSystem;
	boost::get<1>(result) = tradeSystem;
	return result;
}
