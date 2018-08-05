//
//    Created: 2018/07/27 10:54 PM
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
namespace Huobi {

struct AuthSettings : Rest::Settings {
  std::string apiKey;
  std::string apiSecret;
  std::string account;

  explicit AuthSettings(const boost::property_tree::ptree &, ModuleEventsLog &);
};

}  // namespace Huobi
}  // namespace Interaction
}  // namespace trdk