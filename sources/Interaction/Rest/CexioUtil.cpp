/*******************************************************************************
 *   Created: 2018/01/18 00:38:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CexioUtil.hpp"
#include "CexioRequest.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

namespace {
template <typename Value>
boost::optional<Value> ParseOptionalValue(const ptr::ptree &source,
                                          const char *key) {
  const auto &value = source.get<std::string>(key);
  if (value == "null") {
    return boost::none;
  }
  return boost::lexical_cast<Value>(value);
}

std::string FormatRequest(size_t qtyPrecision, size_t pricePrecision) {
  return "type=%1%&amount=%2$." +
         boost::lexical_cast<std::string>(qtyPrecision) + "f&price=%3$." +
         boost::lexical_cast<std::string>(pricePrecision) + 'f';
}
}  // namespace

boost::unordered_map<std::string, CexioProduct> Rest::RequestCexioProductList(
    std::unique_ptr<net::HTTPSClientSession> &session,
    const Context &context,
    ModuleEventsLog &log) {
  boost::unordered_map<std::string, CexioProduct> result;

  ptr::ptree response;
  try {
    response = boost::get<1>(
        CexioPublicRequest("currency_limits", std::string(), context, log)
            .Send(session));
    if (response.get<std::string>("ok") != "ok") {
      throw Exception("Failed to request product list");
    }
    for (const auto &node : response.get_child("data.pairs")) {
      const auto &pair = node.second;
      const auto &symbol1 = pair.get<std::string>("symbol1");
      const auto &symbol2 = pair.get<std::string>("symbol2");
      std::string requestParamsFormat;
      if (symbol1 == "BTC") {
      } else if (symbol1 == "ETH") {
        requestParamsFormat = FormatRequest(8, 1);
      } else if (symbol1 == "BCH") {
        if (symbol2 == "BTC") {
          requestParamsFormat = FormatRequest(8, 6);
        } else {
          requestParamsFormat = FormatRequest(8, 2);
        }
      } else if (symbol1 == "BTG") {
        if (symbol2 == "BTC") {
          requestParamsFormat = FormatRequest(8, 6);
        } else {
          requestParamsFormat = FormatRequest(8, 2);
        }
      } else if (symbol1 == "DASH") {
        if (symbol2 == "BTC") {
          requestParamsFormat = FormatRequest(8, 6);
        } else {
          requestParamsFormat = FormatRequest(8, 2);
        }
      } else if (symbol1 == "XRP") {
        if (symbol2 == "BTC") {
          requestParamsFormat = FormatRequest(6, 8);
        } else {
          requestParamsFormat = FormatRequest(6, 4);
        }
      } else {
        requestParamsFormat = FormatRequest(8, 8);
      }
      result.emplace(symbol1 + '_' + symbol2,
                     CexioProduct{symbol1 + '/' + symbol2,
                                  ParseOptionalValue<Qty>(pair, "minLotSize"),
                                  ParseOptionalValue<Qty>(pair, "minLotSizeS2"),
                                  ParseOptionalValue<Qty>(pair, "maxLotSize"),
                                  ParseOptionalValue<Price>(pair, "minPrice"),
                                  ParseOptionalValue<Price>(pair, "maxPrice"),
                                  std::move(requestParamsFormat)});
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

pt::ptime Rest::ParseCexioTimeStamp(const ptr::ptree &source,
                                    const pt::time_duration &serverTimeDiff) {
  auto result = pt::from_time_t(source.get<time_t>("timestamp"));
  result -= serverTimeDiff;
  return result;
}

std::unique_ptr<net::HTTPSClientSession> Rest::CreateCexioSession(
    const Settings &settings, bool isTrading) {
  return CreateSession("cex.io", settings, isTrading);
}
