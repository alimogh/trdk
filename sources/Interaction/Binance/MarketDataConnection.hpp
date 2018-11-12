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

#include "Product.hpp"

namespace trdk {
namespace Interaction {
namespace Binance {

class MarketDataConnection : public Lib::WebSocketConnection {
 public:
  MarketDataConnection();
  void Connect();
  void Start(const boost::unordered_map<ProductId,
                                        boost::shared_ptr<Rest::Security>> &,
             const Events &);
};
}  // namespace Binance
}  // namespace Interaction
}  // namespace trdk
