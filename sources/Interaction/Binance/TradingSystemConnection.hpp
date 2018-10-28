//
//    Created: 2018/10/27 12:41 AM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "WebSocketConnection.hpp"

namespace trdk {
namespace Interaction {
namespace Binance {

class TradingSystemConnection : public WebSocketConnection {
 public:
  TradingSystemConnection();
  void Connect();
  void Start(const std::string &key, const Events &, Context &);
};
}  // namespace Binance
}  // namespace Interaction
}  // namespace trdk