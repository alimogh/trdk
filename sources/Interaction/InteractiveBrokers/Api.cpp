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
trdk::TradingSystemFactoryResult CreateTradingSystem(
		const trdk::TradingMode &mode,
		size_t index,
		trdk::Context &context,
		const std::string &tag,
		const trdk::Lib::IniSectionRef &configuration) {
	trdk::TradingSystemFactoryResult result;
	const auto &tradingSystem = boost::make_shared<ib::TradingSystem>(
		mode,
		index,
		context,
		tag,
		configuration);
	boost::get<0>(result) = tradingSystem;
	boost::get<1>(result) = tradingSystem;
	return result;
}
