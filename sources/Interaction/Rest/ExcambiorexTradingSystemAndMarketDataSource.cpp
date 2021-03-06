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
using namespace Lib;
using namespace Interaction::Rest;

trdk::TradingSystemAndMarketDataSourceFactoryResult CreateExcambiorex(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const boost::property_tree::ptree &conf) {
  auto tradingSystem = boost::make_shared<ExcambiorexTradingSystem>(
      App::GetInstance(), mode, context, instanceName, title, conf);
  return {std::move(tradingSystem),
          boost::make_shared<ExcambiorexMarketDataSource>(
              App::GetInstance(), context, std::move(instanceName),
              std::move(title), conf)};
}
