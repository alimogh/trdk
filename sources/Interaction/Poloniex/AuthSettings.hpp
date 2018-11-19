//
//    Created: 2018/11/19 16:07
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
namespace Poloniex {

struct AuthSettings : Rest::Settings {
  std::string apiKey;
  std::string apiSecret;

  explicit AuthSettings(const boost::property_tree::ptree &, ModuleEventsLog &);
};

}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk