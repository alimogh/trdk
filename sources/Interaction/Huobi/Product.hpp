//
//    Created: 2018/07/27 6:53 PM
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
namespace Huobi {

typedef std::string ProductId;

struct OrderRequirements {
  Qty minQty;
  Qty maxQty;
};

struct Product {
  ProductId id;
  boost::optional<OrderRequirements> orderRequirements;
  uint8_t pricePrecision;
  uint8_t qtyPrecision;
};

boost::unordered_map<std::string, Product> RequestProductList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &,
    const Context &,
    ModuleEventsLog &);

}  // namespace Huobi
}  // namespace Interaction
}  // namespace trdk