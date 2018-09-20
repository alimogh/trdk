/*******************************************************************************
 *   Created: 2017/11/19 18:26:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BittrexUtil.hpp"
#include "BittrexRequest.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
namespace net = Poco::Net;

namespace {

void ReplaceSymbolWithAlias(std::string &symbol, const Settings &settings) {
  if (symbol == "BCC") {
    symbol = "BCH";
  }
  settings.ReplaceSymbolWithAlias(symbol);
}

std::string NormilizeSymbol(std::string source, const Settings &settings) {
  std::vector<std::string> currencies;
  boost::split(currencies, source, boost::is_any_of("-"));
  AssertEq(2, currencies.size());
  if (currencies.size() != 2) {
    ReplaceSymbolWithAlias(source, settings);
    return source;
  }
  for (auto &currency : currencies) {
    ReplaceSymbolWithAlias(currency, settings);
  }
  currencies[0].swap(currencies[1]);
  return boost::join(currencies, "_");
}
}  // namespace

boost::unordered_map<std::string, Rest::BittrexProduct>
Rest::RequestBittrexProductList(
    std::unique_ptr<net::HTTPSClientSession> &session,
    const Context &context,
    ModuleEventsLog &log) {
  boost::unordered_map<std::string, BittrexProduct> result;
  BittrexPublicRequest request("getmarkets", std::string(), context, log);
  try {
    const auto response = boost::get<1>(request.Send(session));
    for (const auto &node : response) {
      const auto &data = node.second;
      BittrexProduct product = {data.get<std::string>("MarketName"),
                                data.get<Qty>("MinTradeSize")};
      auto symbol = NormilizeSymbol(product.id, context.GetSettings());
      if (!data.get<bool>("IsActive")) {
        log.Warn(R"(Symbol "%1%" is inactive.)", symbol);
        continue;
      }
      const auto &productIt =
          result.emplace(std::move(symbol), std::move(product));
      if (!productIt.second) {
        log.Error("Product duplicate: \"%1%\"", productIt.first->first);
        Assert(productIt.second);
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