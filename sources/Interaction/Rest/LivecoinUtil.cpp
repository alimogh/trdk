/*******************************************************************************
 *   Created: 2017/12/12 15:38:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LivecoinUtil.hpp"
#include "LivecoinRequest.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace ptr = boost::property_tree;

boost::unordered_map<std::string, LivecoinProduct>
Rest::RequestLivecoinProductList(net::HTTPClientSession &session,
                                 const Context &context,
                                 ModuleEventsLog &log) {
  boost::unordered_map<std::string, LivecoinProduct> result;

  ptr::ptree response;
  try {
    response = boost::get<1>(LivecoinPublicRequest("/exchange/restrictions",
                                                   std::string(), context, log)
                                 .Send(session));
    if (!response.get<bool>("success")) {
      throw Exception("Failed to request product list");
    }
    const auto &minBtcVolume = response.get<Volume>("minBtcVolume");
    for (const auto &node : response.get_child("restrictions")) {
      const LivecoinProduct product = {
          node.second.get<std::string>("currencyPair"),
          boost::replace_first_copy(product.id, "/", "%2F"), minBtcVolume,
          node.second.get<uint8_t>("priceScale")};
      const auto &symbol = boost::replace_first_copy(product.id, "/", "_");
      if (!result.emplace(std::move(symbol), std::move(product)).second) {
        log.Error("Product duplicate: \"%1%\"",
                  node.second.get<std::string>("symbol"));
        continue;
      }
    }
  } catch (const std::exception &ex) {
    log.Error(
        "Failed to read supported product list: \"%1%\". Message: \"%2%\".",
        ex.what(),                          // 1
        ConvertToString(response, false));  // 2

    throw Exception(ex.what());
  }
  if (result.empty()) {
    throw Exception("Exchange doesn't have products");
  }
  {
    std::string listStr;
    for (const auto &item : result) {
      if (!listStr.empty()) {
        listStr += ", ";
      }
      listStr += item.first + " (" + item.second.id + ")";
    }
    log.Debug("Supported symbol list: %1%.", listStr);
  }
  return result;
}
