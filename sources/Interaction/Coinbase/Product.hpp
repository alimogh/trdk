//
//    Created: 2018/11/12 11:09 PM
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
namespace Coinbase {

typedef std::string ProductId;

struct Product {
  ProductId id;
  Qty minQty;
  Qty maxQty;
  uintmax_t precisionPower;
};

struct SecuritySubscription {
  boost::shared_ptr<Rest::Security> security;
  std::map<Price, std::pair<Level1TickValue, Level1TickValue>> bids;
  std::map<Price, std::pair<Level1TickValue, Level1TickValue>> asks;
};

}  // namespace Coinbase
}  // namespace Interaction
}  // namespace trdk
