//
//    Created: 2018/04/07 3:47 PM
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
using namespace Exmo;
using namespace Crypto;
namespace net = Poco::Net;
namespace ptr = boost::property_tree;

Request::Request(const std::string &name,
                 const std::string &method,
                 const std::string &params,
                 const Context &context,
                 ModuleEventsLog &eventsLog,
                 ModuleTradingLog *tradingLog)
    : Base(
          "/v1/" + name, name, method, params, context, eventsLog, tradingLog) {
}

Rest::FloodControl &Request::GetFloodControl() const {
  static auto result = Rest::CreateDisabledFloodControl();
  return *result;
}

PublicRequest::PublicRequest(const std::string &name,
                             const std::string &params,
                             const Context &context,
                             ModuleEventsLog &log)
    : Request(name, net::HTTPRequest::HTTP_GET, params, context, log) {}

bool PublicRequest::IsPriority() const { return false; }

PrivateRequest::PrivateRequest(const std::string &name,
                               const std::string &params,
                               const Context &context,
                               const AuthSettings &settings,
                               Rest::NonceStorage &nonces,
                               const bool isPriority,
                               ModuleEventsLog &log,
                               ModuleTradingLog *tradingLog)
    : Request(
          name, net::HTTPRequest::HTTP_POST, params, context, log, tradingLog),
      m_settings(settings),
      m_nonceStorage(nonces),
      m_isPriority(isPriority) {}

PrivateRequest::Response PrivateRequest::Send(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &session) {
  Assert(!m_nonce);
  m_nonce.emplace(m_nonceStorage.TakeNonce());
  const auto &result = Base::Send(session);
  Assert(m_nonce);
  m_nonce = boost::none;
  CheckResponseError(boost::get<1>(result));
  return result;
}

void PrivateRequest::CheckResponseError(const ptr::ptree &response) const {
  {
    const auto &resultField = response.get_optional<bool>("result");
    if (!resultField || *resultField) {
      return;
    }
  }

  const auto &errorMessage = response.get<std::string>("error");

  std::ostringstream error;
  error << "The server returned an error in response to the request \""
        << GetName() << "\" (" << GetRequest().getURI() << "): "
        << "\"" << errorMessage << "\"";

  boost::starts_with(errorMessage, "Error 50304: Order was not found ")
      ? throw OrderIsUnknownException(error.str().c_str())
      : errorMessage == "Error 50052: Insufficient funds"
            ? throw InsufficientFundsException(error.str().c_str())
            : throw Exception(error.str().c_str());
}

bool PrivateRequest::IsPriority() const { return m_isPriority; }

void PrivateRequest::CreateBody(const net::HTTPClientSession &session,
                                std::string &result) const {
  Assert(m_nonce);
  AppendUriParams("nonce=" + boost::lexical_cast<std::string>(m_nonce->Get()),
                  result);
  Base::CreateBody(session, result);
}

void PrivateRequest::PrepareRequest(const net::HTTPClientSession &session,
                                    const std::string &body,
                                    net::HTTPRequest &request) const {
  request.set("Key", m_settings.apiKey);
  {
    const auto &digest = Hmac::CalcSha512Digest(body, m_settings.apiSecret);
    request.set("Sign", EncodeToHex(&digest[0], digest.size()));
  }
  Base::PrepareRequest(session, body, request);
}

TradingRequest::TradingRequest(const std::string &name,
                               const std::string &params,
                               const Context &context,
                               const AuthSettings &settings,
                               Rest::NonceStorage &nonces,
                               ModuleEventsLog &eventsLog,
                               ModuleTradingLog &tradingLog)
    : PrivateRequest(name,
                     params,
                     context,
                     settings,
                     nonces,
                     true,
                     eventsLog,
                     &tradingLog) {}