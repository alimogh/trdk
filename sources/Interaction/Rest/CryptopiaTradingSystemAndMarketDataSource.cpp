/*******************************************************************************
 *   Created: 2017/12/08 03:40:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CryptopiaMarketDataSource.hpp"
#include "CryptopiaTradingSystem.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction::Rest;

TradingSystemAndMarketDataSourceFactoryResult CreateCryptopia(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const boost::property_tree::ptree &conf) {
  auto tradingSystem = boost::make_shared<CryptopiaTradingSystem>(
      App::GetInstance(), mode, context, instanceName, title, conf);
  return {std::move(tradingSystem),
          boost::make_shared<CryptopiaMarketDataSource>(
              App::GetInstance(), context, std::move(instanceName),
              std::move(title), conf)};
}
