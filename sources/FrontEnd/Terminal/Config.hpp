//
//    Created: 2018/05/26 2:05 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace FrontEnd {

class CheckConfigException : public Lib::Exception {
 public:
  explicit CheckConfigException(const char *what) noexcept : Exception(what) {}
};

void CheckConfig(const boost::filesystem::path &configFilePath);
}  // namespace FrontEnd
}  // namespace trdk
