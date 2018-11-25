//
//    Created: 2018/11/23 13:53
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "Common/WebSocketConnection.hpp"

namespace trdk {
namespace TradingLib {

class WebSocketConnection : public Lib::WebSocketConnection {
 public:
  using Base = Lib::WebSocketConnection;

  using Lib::WebSocketConnection::WebSocketConnection;
  WebSocketConnection(WebSocketConnection &&) = default;
  WebSocketConnection(const WebSocketConnection &) = delete;
  WebSocketConnection &operator=(WebSocketConnection &&) = delete;
  WebSocketConnection &operator=(const WebSocketConnection &) = delete;
  virtual ~WebSocketConnection() = default;

  virtual void Connect() = 0;
  virtual void StartData(const Events &) = 0;

 protected:
  using Base::Connect;
};
}  // namespace TradingLib
}  // namespace trdk