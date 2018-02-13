/*******************************************************************************
 *   Created: 2018/02/11 21:45:29
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "ExcambiorexMarketDataSource.hpp"
#include "ExcambiorexTradingSystem.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

trdk::TradingSystemAndMarketDataSourceFactoryResult CreateExcambiorex(
    const trdk::TradingMode &mode,
    trdk::Context &context,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &conf) {
  return {boost::make_shared<ExcambiorexTradingSystem>(
              App::GetInstance(), mode, context, instanceName, conf),
          boost::make_shared<ExcambiorexMarketDataSource>(
              App::GetInstance(), context, instanceName, conf)};
}
