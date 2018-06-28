//
//    Created: 2018/04/07 3:47 PM
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
using namespace Exmo;
namespace net = Poco::Net;
namespace ptr = boost::property_tree;

boost::unordered_map<std::string, Product> Exmo::RequestProductList(
    std::unique_ptr<net::HTTPSClientSession> &session,
    const Context &context,
    ModuleEventsLog &log) {
  boost::unordered_map<std::string, Product> result;
  ptr::ptree response;
  try {
    response = boost::get<1>(
        PublicRequest("pair_settings", "", context, log).Send(session));
    for (const auto &symbol : response) {
      const auto &data = symbol.second;
      result.emplace(
          symbol.first,
          Product{symbol.first, data.get<Qty>("min_quantity"),
                  data.get<Qty>("max_quantity"), data.get<Price>("min_price"),
                  data.get<Price>("max_price"), data.get<Volume>("min_amount"),
                  data.get<Volume>("max_amount")});
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
