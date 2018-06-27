/*******************************************************************************
 *   Created: 2017/12/19 21:16:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LivecoinMarketDataSource.hpp"
#include "LivecoinTradingSystem.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction::Rest;

trdk::TradingSystemAndMarketDataSourceFactoryResult CreateLivecoin(
    const trdk::TradingMode &mode,
    trdk::Context &context,
    std::string instanceName,
    std::string title,
    const boost::property_tree::ptree &conf) {
  auto tradingSystem = boost::make_shared<LivecoinTradingSystem>(
      App::GetInstance(), mode, context, instanceName, title, conf);
  return {std::move(tradingSystem),
          boost::make_shared<LivecoinMarketDataSource>(
              App::GetInstance(), context, std::move(instanceName),
              std::move(title), conf)};
}
