//
//    Created: 2018/10/14 8:22 PM
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
namespace Kraken {

struct AuthSettings : Rest::Settings {
  std::string apiKey;
  std::vector<unsigned char> apiSecret;

  explicit AuthSettings(const boost::property_tree::ptree &, ModuleEventsLog &);
};

}  // namespace Kraken
}  // namespace Interaction
}  // namespace trdk