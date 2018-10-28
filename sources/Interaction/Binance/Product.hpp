//
//    Created: 2018/10/24 08:49
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
namespace Binance {

typedef std::string ProductId;

struct Product {
  struct Filter {
    Price min;
    Price max;
    uintmax_t precisionPower;
  };

  ProductId id;
  boost::optional<Filter> priceFilter;
  boost::optional<Filter> qtyFilter;
  boost::optional<Qty> minVolume;
};

std::string ResolveSymbol(const std::string &);

boost::unordered_map<std::string, Product> GetProductList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &,
    const Context &,
    ModuleEventsLog &);

}  // namespace Binance
}  // namespace Interaction
}  // namespace trdk