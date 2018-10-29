
#include "Prec.hpp"
#include "AuthSettings.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Binance;
namespace ptr = boost::property_tree;

AuthSettings::AuthSettings(const ptr::ptree &conf, ModuleEventsLog &log)
    : Settings(conf, log),
      apiKey(conf.get<std::string>("config.auth.apiKey")),
      apiSecret(conf.get<std::string>("config.auth.apiSecret")) {
  log.Info(R"(API key: "%1%". API secret: %2%.)",
           apiKey,                                     // 1
           apiSecret.empty() ? "not set" : "is set");  // 2
}
