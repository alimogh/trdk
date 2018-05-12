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

namespace trdk {
namespace Interaction {
namespace Exmo {

typedef std::string ProductId;

struct Product {
  ProductId id;
  Qty minQty;
  Qty maxQty;
  Price minPrice;
  Price maxPrice;
  Volume maxVolume;
  Volume minVolume;
};

boost::unordered_map<std::string, Product> RequestProductList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &,
    const Context &,
    ModuleEventsLog &);

}  // namespace Exmo
}  // namespace Interaction
}  // namespace trdk