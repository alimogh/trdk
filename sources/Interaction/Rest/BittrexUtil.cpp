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
using namespace trdk::Lib;
using namespace trdk::Interaction;
namespace net = Poco::Net;

namespace {
std::string NormilizeSymbol(std::string source) {
  if (boost::starts_with(source, "BCC-")) {
    source[2] = 'H';
  } else if (boost::ends_with(source, "-BCC")) {
    source.back() = 'H';
  }
  std::vector<std::string> subs;
  boost::split(subs, source, boost::is_any_of("-"));
  if (subs.size() != 2) {
    return source;
  }
  subs[0].swap(subs[1]);
  return boost::join(subs, "_");
}
}

boost::unordered_map<std::string, Rest::BittrexProduct>
Rest::RequestBittrexProductList(net::HTTPClientSession &session,
                                const Context &context,
                                ModuleEventsLog &log) {
  boost::unordered_map<std::string, BittrexProduct> result;
  BittrexPublicRequest request("getmarkets", std::string(), context, log);
  try {
    const auto response = boost::get<1>(request.Send(session));
    for (const auto &node : response) {
      const auto &data = node.second;
      BittrexProduct product = {data.get<std::string>("MarketName")};
      const auto &symbol = NormilizeSymbol(product.id);
      const auto &productIt =
          result.emplace(std::move(symbol), std::move(product));
      if (!productIt.second) {
        log.Error("Product duplicate: \"%1%\"", productIt.first->first);
        Assert(productIt.second);
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