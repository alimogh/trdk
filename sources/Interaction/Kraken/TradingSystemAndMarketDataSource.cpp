//
//    Created: 2018/10/14 9:29 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "MarketDataSource.hpp"
#include "TradingSystem.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Rest;

TradingSystemAndMarketDataSourceFactoryResult
CreateTradingSystemAndMarketDataSource(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const boost::property_tree::ptree &conf) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  auto tradingSystem = boost::make_shared<Kraken::TradingSystem>(
      App::GetInstance(), mode, context, instanceName, title, conf);
  return {std::move(tradingSystem),
          boost::make_shared<Kraken::MarketDataSource>(
              App::GetInstance(), context, std::move(instanceName),
              std::move(title), conf)};
}
