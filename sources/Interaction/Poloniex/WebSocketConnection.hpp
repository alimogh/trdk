//
//    Created: 2018/11/19 14:52
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

class WebSocketConnection : public TradingLib::WebSocketConnection {
 public:
  using Base = TradingLib::WebSocketConnection;

  WebSocketConnection();
  ~WebSocketConnection() override = default;

  void Connect() override;
};
}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk
