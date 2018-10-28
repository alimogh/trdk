//
//    Created: 2018/10/24 08:50
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "Product.hpp"
#include "Request.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Binance;
namespace net = Poco::Net;
namespace ptr = boost::property_tree;

namespace {
boost::unordered_map<std::string, Product> RequestProductList(
    std::unique_ptr<net::HTTPSClientSession> &session,
    const Context &context,
    ModuleEventsLog &log) {
  boost::unordered_map<std::string, Product> result;
  ptr::ptree response;
  try {
    response = boost::get<1>(
        PublicRequest("v1/exchangeInfo", "", context, log).Send(session));
    for (const auto &symbolNode : response.get_child("symbols")) {
      const auto &data = symbolNode.second;
      auto symbol = data.get<std::string>("baseAsset") + "_" +
                    data.get<std::string>("quoteAsset");
      Product product{data.get<std::string>("symbol")};
      {
        const auto &status = data.get<std::string>("status");
        if (status != "TRADING") {
          log.Debug(R"(Symbol "%1%" has status "%2%".)", symbol,  // 1
                    status);                                      // 2
          continue;
        }
      }

      for (const auto &filterNode : data.get_child("filters")) {
        const auto &filter = filterNode.second;
        const auto &type = filter.get<std::string>("filterType");
        if (type == "PRICE_FILTER") {
          Assert(!product.priceFilter);
          product.priceFilter.emplace(Product::Filter{
              filter.get<Price>("minPrice"), filter.get<Price>("maxPrice"), 0});
        } else if (type == "LOT_SIZE") {
          product.qtyFilter.emplace(Product::Filter{
              filter.get<Price>("minQty"), filter.get<Price>("maxQty"), 0});
        } else if (type == "MIN_NOTIONAL") {
          product.minVolume.emplace(filter.get<Qty>("minNotional"));
        }
      }

      result.emplace(std::move(symbol), std::move(product));
    }
  } catch (const std::exception &ex) {
    log.Error(
        R"(Failed to read supported product list: "%1%". Message: "%2%".)",
        ex.what(),                                // 1
        Rest::ConvertToString(response, false));  // 2
    throw Exception(ex.what());
  }
  if (result.empty()) {
    throw Exception("Exchange doesn't have products");
  }
  return result;
}
}  // namespace

boost::unordered_map<std::string, Product> Binance::GetProductList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &session,
    const Context &context,
    ModuleEventsLog &log) {
  static const auto result = RequestProductList(session, context, log);
  return result;
}
