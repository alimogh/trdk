//
//    Created: 2018/11/10
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "Product.hpp"

namespace trdk {
namespace Interaction {
namespace Coinbase {

class MarketDataConnection : public Lib::WebSocketConnection {
 public:
  MarketDataConnection();
  void Connect();
  void Start(const boost::unordered_map<ProductId, SecuritySubscription> &,
             const Events &);
};
}  // namespace Coinbase
}  // namespace Interaction
}  // namespace trdk
