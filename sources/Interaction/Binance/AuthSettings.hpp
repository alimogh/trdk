//
//    Created: 2018/10/27 12:57 AM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace Interaction {
namespace Binance {

struct AuthSettings : Rest::Settings {
  std::string apiKey;
  std::string apiSecret;

  explicit AuthSettings(const boost::property_tree::ptree &, ModuleEventsLog &);
};

}  // namespace Binance
}  // namespace Interaction
}  // namespace trdk