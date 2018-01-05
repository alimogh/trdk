/*******************************************************************************
 *   Created: 2017/12/07 20:56:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CryptopiaUtil.hpp"
#include "CryptopiaRequest.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;

CryptopiaProductList Rest::RequestCryptopiaProductList(
    net::HTTPClientSession &session,
    const Context &context,
    ModuleEventsLog &log) {
  CryptopiaProductList result;
  CryptopiaPublicRequest request("GetTradePairs", std::string(), context, log);
  try {
    const auto response = boost::get<1>(request.Send(session));
    for (const auto &node : response) {
      const auto &pairNode = node.second;
      auto quoteSymbol = pairNode.get<std::string>("Symbol");
      auto baseSymbol = pairNode.get<std::string>("BaseSymbol");
#if 0
      const bool isReverted = baseSymbol == "LTC" && quoteSymbol == "ETH";
      if (isReverted) {
        quoteSymbol.swap(baseSymbol);
      }
#else
      const bool isReverted = false;
#endif
      const auto symbol = quoteSymbol + "_" + baseSymbol;
      const CryptopiaProduct product = {
          pairNode.get<CryptopiaProductId>("Id"),
          std::move(symbol),
          isReverted,
          pairNode.get<Double>("TradeFee") / 100,
          {pairNode.get<Qty>("MinimumTrade"),
           pairNode.get<Qty>("MaximumTrade")},
          {pairNode.get<Volume>("MinimumBaseTrade"),
           pairNode.get<Volume>("MaximumBaseTrade")},
          {pairNode.get<Price>("MinimumPrice"),
           pairNode.get<Price>("MaximumPrice")}};
      if (!result.emplace(std::move(product)).second) {
        log.Error("Product duplicate: \"%1%\"",
                  pairNode.get<std::string>("Symbol") + "_" +
                      pairNode.get<std::string>("BaseSymbol"));
        continue;
      }
    }
  } catch (const std::exception &ex) {
    log.Error("Failed to read supported product list: \"%1%\".", ex.what());
    throw Exception(ex.what());
  }
  if (result.empty()) {
    throw Exception("Exchange doesn't have products");
  }
  return result;
}
