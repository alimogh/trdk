//
//    Created: 2018/11/19 16:49
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
using namespace Poloniex;
namespace ptr = boost::property_tree;

AuthSettings::AuthSettings(const ptr::ptree &conf, ModuleEventsLog &log)
    : Settings(conf, log),
      apiKey(conf.get<std::string>("config.auth.apiKey")),
      apiSecret(conf.get<std::string>("config.auth.apiSecret")) {
  log.Info(R"(API key: "%1%". API secret: %2%.)",
           apiKey,                                     // 1
           apiSecret.empty() ? "not set" : "is set");  // 2
}
