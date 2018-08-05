//
//    Created: 2018/07/27 7:13 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "Request.hpp"
#include "AuthSettings.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Huobi;
using namespace Crypto;
namespace net = Poco::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

Request::Request(const std::string &name,
                 const std::string &method,
                 std::string params,
                 const Context &context,
                 ModuleEventsLog &eventsLog,
                 ModuleTradingLog *tradingLog)
    : Base("/" + name,
           name,
           method,
           std::move(params),
           context,
           eventsLog,
           tradingLog,
           "application/json") {}

Rest::FloodControl &Request::GetFloodControl() const {
  static auto result = Rest::CreateDisabledFloodControl();
  return *result;
}

Request::Response Request::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  auto result = Base::Send(session);
  {
    const auto &response = boost::get<1>(result);
    if (response.get<std::string>("status") != "ok") {
      CheckResponseError(response);
    }
  }
  return result;
}

void Request::CheckResponseError(const ptr::ptree &response) const {
  boost::format error(
      "The server returned an error in response to the request \"%1%\" (%2%): "
      "\"%3%\" (code: \"%4%\")");
  error % GetName()                                             // 1
      % GetRequest().getURI()                                   // 2
      % response.get<std::string>("err-msg", "UNKNOWN ERROR")   // 3
      % response.get<std::string>("err-code", "UNKNOWN CODE");  // 4
  throw Exception(error.str().c_str());
}

PublicRequest::PublicRequest(const std::string &name,
                             std::string params,
                             const Context &context,
                             ModuleEventsLog &log)
    : Request(
          name, net::HTTPRequest::HTTP_GET, std::move(params), context, log) {}

bool PublicRequest::IsPriority() const { return false; }

PrivateRequest::PrivateRequest(const std::string &name,
                               const std::string &method,
                               std::string params,
                               const Context &context,
                               const AuthSettings &settings,
                               const bool isPriority,
                               ModuleEventsLog &log,
                               ModuleTradingLog *tradingLog)
    : Request(name, method, {}, context, log, tradingLog),
      m_params(std::move(params)),
      m_settings(settings),
      m_isPriority(isPriority) {}

bool PrivateRequest::IsPriority() const { return m_isPriority; }

void PrivateRequest::CreateBody(const net::HTTPClientSession &session,
                                std::string &result) const {
  if (GetRequest().getMethod() != net::HTTPRequest::HTTP_POST) {
    Base::CreateBody(session, result);
    return;
  }
  result = m_params;
}

void PrivateRequest::WriteUri(std::string uri,
                              net::HTTPRequest &request) const {
  if (request.getMethod() != net::HTTPRequest::HTTP_POST) {
    Base::WriteUri(std::move(uri), request);
    return;
  }
  const auto &params = GetUriParams();
  if (!params.empty()) {
    uri += '?' + params;
  }
  request.setURI(uri);
}

PrivateRequest::Response PrivateRequest::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  auto timestamp =
      pt::to_iso_extended_string(pt::second_clock::universal_time());
  boost::replace_all(timestamp, ":", "%3A");
  auto params =
      "AccessKeyId=" + m_settings.apiKey +
      "&SignatureMethod=HmacSHA256&SignatureVersion=2&Timestamp=" + timestamp;
  if (!m_params.empty() &&
      GetRequest().getMethod() != net::HTTPRequest::HTTP_POST) {
    params += '&' + m_params;
  }
  const auto controlString = GetRequest().getMethod() + "\n" +
                             session->getHost() + "\n" + GetUri() + "\n" +
                             params;
  const auto &digest =
      Hmac::CalcSha256Digest(controlString, m_settings.apiSecret);
  const auto sign = Base64::Encode(digest, false);
  std::string encodedSign;
  Poco::URI::encode(sign, "+/=", encodedSign);
  SetUriParams(params + "&Signature=" + encodedSign);
  return Base::Send(session);
}

TradingRequest::TradingRequest(const std::string &name,
                               std::string params,
                               const Context &context,
                               const AuthSettings &settings,
                               ModuleEventsLog &eventsLog,
                               ModuleTradingLog &tradingLog)
    : PrivateRequest(name,
                     net::HTTPRequest::HTTP_POST,
                     std::move(params),
                     context,
                     settings,
                     true,
                     eventsLog,
                     &tradingLog) {}