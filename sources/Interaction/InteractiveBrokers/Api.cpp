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
#include "Api.h"
#include "IbTradingSystem.hpp"

namespace ib = trdk::Interaction::InteractiveBrokers;

TRDK_INTERACTION_INTERACTIVEBROKERS_API
trdk::TradingSystemAndMarketDataSourceFactoryResult
CreateTradingSystemAndMarketDataSource(
    const trdk::TradingMode &mode,
    trdk::Context &context,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &configuration) {
  const auto &object = boost::make_shared<ib::TradingSystem>(
      mode, context, instanceName, configuration);
  const trdk::TradingSystemAndMarketDataSourceFactoryResult result = {object,
                                                                      object};
  return result;
}

TRDK_INTERACTION_INTERACTIVEBROKERS_API
boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
    const trdk::TradingMode &mode,
    trdk::Context &context,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &configuration) {
  const auto &result = boost::make_shared<ib::TradingSystem>(
      mode, context, instanceName, configuration);
  return result;
}
