//
//    Created: 2018/04/07 3:47 PM
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
CreateTradingSystemAndMarketDataSource(const TradingMode &mode,
                                       Context &context,
                                       const std::string &instanceName,
                                       const IniSectionRef &conf) {
  return {boost::make_shared<Exmo::TradingSystem>(App::GetInstance(), mode,
                                                  context, instanceName, conf),
          boost::make_shared<Exmo::MarketDataSource>(
              App::GetInstance(), context, instanceName, conf)};
}
