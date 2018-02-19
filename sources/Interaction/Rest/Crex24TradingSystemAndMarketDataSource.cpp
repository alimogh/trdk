/*******************************************************************************
 *   Created: 2018/02/16 03:57:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Crex24MarketDataSource.hpp"
#include "Crex24TradingSystem.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

trdk::TradingSystemAndMarketDataSourceFactoryResult CreateCrex24(
    const trdk::TradingMode &mode,
    trdk::Context &context,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &conf) {
  return {boost::make_shared<Crex24TradingSystem>(App::GetInstance(), mode,
                                                  context, instanceName, conf),
          boost::make_shared<Crex24MarketDataSource>(
              App::GetInstance(), context, instanceName, conf)};
}
