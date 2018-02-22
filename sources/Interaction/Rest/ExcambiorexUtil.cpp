/*******************************************************************************
 *   Created: 2018/02/11 18:10:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "ExcambiorexUtil.hpp"
#include "ExcambiorexRequest.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

std::pair<ExcambiorexProductList,
          boost::unordered_map<std::string, std::string>>
Rest::RequestExcambiorexProductAndCurrencyList(
    std::unique_ptr<net::HTTPSClientSession> &session,
    const Context &context,
    ModuleEventsLog &log) {
  std::pair<ExcambiorexProductList,
            boost::unordered_map<std::string, std::string>>
      result;

  const auto &addCurrency = [&result, &log](const std::string &id,
                                            const std::string &symbol) {
    const auto &it = result.second.emplace(id, symbol);
    if (!it.second && it.first->second != symbol) {
      log.Error(
          "Duplicate currency ID \"%1%\": received for symbol \"%2%\", but "
          "already added for \"%3%\".",
          id,                 // 1
          symbol,             // 2
          it.first->second);  // 3
      AssertEq(symbol, it.first->second);
    }
  };

  ptr::ptree response;
  try {
    response = boost::get<1>(
        ExcambiorexPublicRequest("Ticker", "", context, log).Send(session));
    for (const auto &node : response) {
      const auto &pairData = node.second;
      const auto &buyCoinAlias = pairData.get<std::string>("buy_coin_alias");
      const auto &sellCoinAlias = pairData.get<std::string>("sell_coin_alias");
      const auto &symbol = buyCoinAlias + "_" + sellCoinAlias;
      const ExcambiorexProduct product = {node.first, symbol, buyCoinAlias,
                                          sellCoinAlias};
      const auto &productIt = result.first.emplace(product);
      if (!productIt.second) {
        log.Error(
            "Duplicate symbol \"%1%\": received with ID \"%2%\", but already "
            "added with ID \"%3%\".",
            symbol,                      // 1
            product.directId,            // 2
            productIt.first->directId);  // 3
        Assert(productIt.second);
      }
      addCurrency(pairData.get<std::string>("buy_coin"), buyCoinAlias);
      addCurrency(pairData.get<std::string>("sell_coin"), sellCoinAlias);
    }
  } catch (const std::exception &ex) {
    log.Error(
        "Failed to read supported product list: \"%1%\". Message: \"%2%\".",
        ex.what(),                          // 1
        ConvertToString(response, false));  // 2
    throw Exception(ex.what());
  }
  if (result.first.empty() || result.second.empty()) {
    throw Exception("Exchange doesn't have products");
  }

  for (auto &product : result.first) {
    AssertEq(std::string(), product.oppositeId);
    for (const auto &oppositeProduct : result.first) {
      if (oppositeProduct.buyCoinAlias == product.sellCoinAlias &&
          oppositeProduct.sellCoinAlias == product.buyCoinAlias) {
        if (!product.oppositeId.empty()) {
          log.Error(
              "Duplicate opposite product \"%1%\" found for product \"%2%\" "
              "that already has opposite product \"%3%\".",
              oppositeProduct.directId,  // 1
              product.directId,          // 2
              product.oppositeId);       // 3
          throw Exception("Failed to build product list");
        }
        const_cast<std::string &>(product.oppositeId) =
            oppositeProduct.directId;
      }
    }
    if (product.oppositeId.empty()) {
      log.Error("Opposite product was not found for product \"%1%\".",
                product.directId);
      throw Exception("Failed to build product list");
    }
  }

  return result;
}

std::unique_ptr<net::HTTPSClientSession> Rest::CreateExcambiorexSession(
    const Settings &settings, bool isTrading) {
  return CreateSession("bit.excambiorex.com", settings, isTrading);
}
