/*******************************************************************************
 *   Created: 2018/02/16 04:00:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Crex24Util.hpp"
#include "Crex24Request.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace ptr = boost::property_tree;

boost::unordered_set<std::string> Rest::RequestCrex24ProductList(
    std::unique_ptr<net::HTTPSClientSession> &session,
    const Context &context,
    ModuleEventsLog &log) {
  boost::unordered_set<std::string> result;
  ptr::ptree response;
  try {
    response = boost::get<1>(
        Crex24PublicRequest("ReturnTicker", "", context, log).Send(session));
    for (const auto &node : response.get_child("Tickers")) {
      const auto &pairData = node.second;
      const auto &symbol = pairData.get<std::string>("PairName");
      Verify(result.emplace(symbol).second);
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
  return result;
}

std::unique_ptr<net::HTTPSClientSession> Rest::CreateCrex24Session(
    const Settings &settings, bool isTrading) {
  return CreateSession("api.crex24.com", settings, isTrading);
}
