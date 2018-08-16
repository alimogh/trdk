//
//    Created: 2018/07/26 10:08 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "AuthSettings.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Huobi;
namespace ptr = boost::property_tree;

AuthSettings::AuthSettings(const ptr::ptree &conf, ModuleEventsLog &log)
    : Rest::Settings(conf, log),
      apiKey(conf.get<std::string>("config.auth.apiKey")),
      apiSecret(conf.get<std::string>("config.auth.apiSecret")),
      account(conf.get<std::string>("config.account")) {
  log.Info(R"(API key: "%1%". API secret: %2%. Account: "%3%".)",
           apiKey,                                    // 1
           apiSecret.empty() ? "not set" : "is set",  // 2
           account);                                  // 3
}
