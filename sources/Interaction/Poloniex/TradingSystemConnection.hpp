//
//    Created: 2018/11/19 14:50
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
namespace Poloniex {

class TradingSystemConnection : public WebSocketConnection {
 public:
  void Start(const AuthSettings &, Rest::NonceStorage &, const Events &);
};
}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk
