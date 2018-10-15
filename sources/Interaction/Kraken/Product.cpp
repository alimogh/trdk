//
//    Created: 2018/10/14 1:26 PM
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
using namespace Kraken;
namespace net = Poco::Net;
namespace ptr = boost::property_tree;

namespace {
Volume GetMinOrderVolume(const std::string &baseCurrency,
                         ModuleEventsLog &log) {
  // https://support.kraken.com/hc/en-us/articles/205893708-What-is-the-minimum-order-size-
  if (baseCurrency == "BTC") {
    // Bitcoin
    return 0.002;
  }
  if (baseCurrency == "BCH") {
    // Bitcoin Cash
    return 0.002;
  }
  if (baseCurrency == "ADA") {
    // Cardano
    return 1;
  }
  if (baseCurrency == "DASH") {
    // Dash
    return 0.03;
  }
  if (baseCurrency == "XDG") {
    // Dogecoin
    return 3000;
  }
  if (baseCurrency == "EOS") {
    // EOS
    return 3;
  }
  if (baseCurrency == "ETH") {
    // Ethereum
    return 0.02;
  }
  if (baseCurrency == "ETC") {
    // Ethereum Classic
    return 0.3;
  }
  if (baseCurrency == "GNO") {
    // Gnosis
    return 0.03;
  }
  if (baseCurrency == "ICN") {
    // Iconomi
    return 2;
  }
  if (baseCurrency == "LTC") {
    // Litecoin
    return 0.1;
  }
  if (baseCurrency == "MLN") {
    // Melon
    return 0.1;
  }
  if (baseCurrency == "XMR") {
    // Monero
    return 0.1;
  }
  if (baseCurrency == "QTUM") {
    // Quantum
    return 0.1;
  }
  if (baseCurrency == "XRP") {
    // Ripple
    return 30;
  }
  if (baseCurrency == "REP") {
    // Augur
    return 0.3;
  }
  if (baseCurrency == "XLM") {
    // Stellar Lumens
    return 30;
  }
  if (baseCurrency == "ZEC") {
    // Zcash
    return 0.03;
  }
  if (baseCurrency == "USDT") {
    // Tether
    return 5;
  }
  log.Warn(
      "Failed to calculate minimal order volume as currency \"%1%\" is "
      "unknown.",
      baseCurrency);
  return 0;
}
}  // namespace

std::string Kraken::ResolveSymbol(const std::string &source) {
  if (source == "XXBT") {
    static const std::string result = "BTC";
    return result;
  }
  if (source.size() == 4 && source[0] == 'X' || source[0] == 'Z') {
    return source.substr(1);
  }
  return source;
}

boost::unordered_map<std::string, Product> Kraken::RequestProductList(
    std::unique_ptr<net::HTTPSClientSession> &session,
    const Context &context,
    ModuleEventsLog &log) {
  boost::unordered_map<std::string, Product> result;
  ptr::ptree response;
  try {
    response = boost::get<1>(
        PublicRequest("AssetPairs", "", context, log).Send(session));
    for (const auto &node : response) {
      if (node.first.find('.') != std::string::npos) {
        continue;
      }
      const auto &data = node.second;
      const auto lotMultiplier = data.get<size_t>("lot_multiplier");
      if (lotMultiplier != 1) {
        AssertEq(1, lotMultiplier);
        continue;
      }
      const auto &base = ResolveSymbol(data.get<std::string>("base"));
      boost::optional<Volume> fee;
      for (const auto &feeNode : data.get_child("fees")) {
        size_t i = 0;
        ;
        for (const auto &feeValueNode : feeNode.second) {
          if (++i < 2) {
            continue;
          }
          const auto &feeValue = feeValueNode.second.get_value<Volume>();
          if (fee.get_value_or(0) < feeValue) {
            fee.emplace(feeValue);
          }
          break;
        }
      }
      result.emplace(base + "_" + ResolveSymbol(data.get<std::string>("quote")),
                     Product{node.first, fee.get_value_or(0) / 100,
                             static_cast<uintmax_t>(std::pow(
                                 10, data.get<uint8_t>("pair_decimals"))),
                             GetMinOrderVolume(base, log)});
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
