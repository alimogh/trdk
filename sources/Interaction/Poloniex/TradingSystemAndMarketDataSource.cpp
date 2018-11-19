//
//    Created: 2018/11/14 16:19
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
using namespace Interaction::Rest;
namespace p = Interaction::Poloniex;

TradingSystemAndMarketDataSourceFactoryResult
CreateTradingSystemAndMarketDataSource(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const boost::property_tree::ptree &config) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  auto tradingSystem = boost::make_shared<p::TradingSystem>(
      App::GetInstance(), mode, context, instanceName, title, config);
  return {std::move(tradingSystem),
          boost::make_shared<p::MarketDataSource>(App::GetInstance(), context,
                                                  std::move(instanceName),
                                                  std::move(title), config)};
}
