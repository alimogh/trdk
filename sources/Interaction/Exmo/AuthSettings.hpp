//
//    Created: 2018/04/07 3:47 PM
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
namespace Exmo {

struct AuthSettings : Rest::Settings {
  std::string apiKey;
  std::string apiSecret;

  explicit AuthSettings(const Lib::IniSectionRef &, ModuleEventsLog &);
};

}  // namespace Exmo
}  // namespace Interaction
}  // namespace trdk