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
using namespace Lib;
using namespace Interaction;
using namespace Rest;

namespace net = Poco::Net;
namespace ptr = boost::property_tree;

boost::unordered_map<std::string, Crex24Product> Rest::RequestCrex24ProductList(
    std::unique_ptr<net::HTTPSClientSession> &session,
    const Context &context,
    ModuleEventsLog &log) {
  boost::unordered_map<std::string, Crex24Product> result;
  ptr::ptree response;
  try {
    response = boost::get<1>(
        Crex24PublicRequest("instruments", "", context, log).Send(session));
    for (const auto &node : response) {
      const auto &pairData = node.second;
      const auto &baseCurrency = pairData.get<std::string>("baseCurrency");
      const auto &quoteCurrency = pairData.get<std::string>("quoteCurrency");
      result.emplace(baseCurrency + "_" + quoteCurrency,
                     Crex24Product{quoteCurrency + "_" + baseCurrency,
                                   pairData.get<Price>("tickSize"),
                                   pairData.get<Price>("minPrice"),
                                   pairData.get<Volume>("minVolume")});
    }
  } catch (const std::exception &ex) {
    log.Error(
        R"(Failed to read supported product list: "%1%". Message: "%2%".)",
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
    const Settings &settings, const bool isTrading) {
  return CreateSession("api.crex24.com", settings, isTrading);
}
