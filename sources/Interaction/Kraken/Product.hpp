//
//    Created: 2018/10/11 08:20
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
namespace Kraken {

typedef std::string ProductId;

struct Product {
  ProductId id;
  Volume feeRatio;
  uintmax_t pricePrecisionPower;
  Volume minVolume;
};

std::string ResolveSymbol(const std::string &);

boost::unordered_map<std::string, Product> RequestProductList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &,
    const Context &,
    ModuleEventsLog &);

}  // namespace Kraken
}  // namespace Interaction
}  // namespace trdk