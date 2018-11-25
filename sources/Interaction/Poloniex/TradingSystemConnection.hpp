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
  explicit TradingSystemConnection(const AuthSettings &, Rest::NonceStorage &);
  ~TradingSystemConnection() override = default;

  void StartData(const Events &) override;

 private:
  const AuthSettings &m_settings;
  Rest::NonceStorage &m_nonceStorage;
};
}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk
