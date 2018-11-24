//
//    Created: 2018/11/14 16:25
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "Product.hpp"
#include "WebSocketConnection.hpp"

namespace trdk {
namespace Interaction {
namespace Poloniex {

class MarketDataConnection : public WebSocketConnection {
 public:
  explicit MarketDataConnection(
      const boost::unordered_map<ProductId, SecuritySubscription> &);
  void StartData(const Events &) override;

 private:
  const boost::unordered_map<ProductId, SecuritySubscription> &m_subscription;
};
}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk
