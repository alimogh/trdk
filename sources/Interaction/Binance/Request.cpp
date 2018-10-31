//
//    Created: 2018/10/24 10:57
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
using namespace Crypto;
using namespace Interaction;
using namespace Binance;
using namespace Rest;
namespace net = Poco::Net;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;

Binance::Request::Request(const std::string &name,
                          const std::string &method,
                          std::string params,
                          const Context &context,
                          ModuleEventsLog &eventsLog,
                          ModuleTradingLog *tradingLog)
    : Base("/api/" + name,
           name,
           method,
           std::move(params),
           context,
           eventsLog,
           tradingLog) {}

FloodControl &Binance::Request::GetFloodControl() const {
  static auto result = CreateDisabledFloodControl();
  return *result;
}

void Binance::Request::CheckErrorResponse(const net::HTTPResponse &response,
                                          const std::string &responseContent,
                                          const size_t attemptNumber) const {
  try {
    const auto &doc = ReadJson(responseContent);
    const auto &code = doc.get_optional<intmax_t>("code");
    if (code) {
      const auto &message = doc.get<std::string>("msg");
      *code == -2010 ? throw InsufficientFundsException(message.c_str())
                     : throw Exception(message.c_str());
    }
  } catch (const ptr::ptree_error &) {
  }
  Base::CheckErrorResponse(response, responseContent, attemptNumber);
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
                               const Context &context,
                               const AuthSettings &settings,
                               ModuleEventsLog &eventsLog,
                               ModuleTradingLog &tradingLog)
    : Request(name, method, "", context, eventsLog, &tradingLog),
      m_settings(settings) {}

bool PrivateRequest::IsPriority() const { return true; }

const AuthSettings &PrivateRequest::GetSettings() const { return m_settings; }

void PrivateRequest::PrepareRequest(const net::HTTPClientSession &session,
                                    const std::string &body,
                                    net::HTTPRequest &request) const {
  request.set("X-MBX-APIKEY", m_settings.apiKey);
  Base::PrepareRequest(session, body, request);
}

SignedRequest::SignedRequest(const std::string &name,
                             const std::string &method,
                             std::string params,
                             const Context &context,
                             const AuthSettings &settings,
                             ModuleEventsLog &eventsLog,
                             ModuleTradingLog &tradingLog)
    : Base(name, method, context, settings, eventsLog, tradingLog),
      m_params(std::move(params)) {
  if (!m_params.empty()) {
    m_params.push_back('&');
  }
}

SignedRequest::Response SignedRequest::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  const auto params =
      m_params + "recvWindow=100000&timestamp=" +
      boost::lexical_cast<std::string>(
          (ConvertToMicroseconds(pt::microsec_clock::universal_time()) / 1000) -
          30000);
  SetUriParams(
      params + "&signature=" +
      EncodeToHex(Hmac::CalcSha256Digest(params, GetSettings().apiSecret)));
  return Base::Send(session);
}
