//
//    Created: 2018/11/22 14:30
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
using namespace Rest;
using namespace Poloniex;
namespace net = Poco::Net;
namespace ptr = boost::property_tree;

FloodControl &Poloniex::Request::GetFloodControl() const {
  static auto result = CreateDisabledFloodControl();
  return *result;
}

void Poloniex::Request::CheckErrorResponse(const net::HTTPResponse &response,
                                           const std::string &responseContent,
                                           const size_t attemptNumber) const {
  try {
    const auto &error =
        ReadJson(responseContent).get_optional<std::string>("error");
    if (error) {
      throw Exception(error->c_str());
    }
  } catch (const ptr::ptree_error &) {
  }
  Base::CheckErrorResponse(response, responseContent, attemptNumber);
}

PrivateRequest::PrivateRequest(const std::string &command,
                               const std::string &params,
                               const Context &context,
                               const AuthSettings &settings,
                               NonceStorage &nonces,
                               ModuleEventsLog &eventsLog,
                               ModuleTradingLog &tradingLog)
    : Request("/tradingApi",
              command,
              net::HTTPRequest::HTTP_POST,
              "",
              context,
              eventsLog,
              &tradingLog),
      m_settings(settings),
      m_nonces(nonces),
      m_params(AppendUriParams("command=" + command, params)) {}

bool PrivateRequest::IsPriority() const { return true; }

PrivateRequest::Response PrivateRequest::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  Assert(!m_nonce);
  m_nonce.emplace(m_nonces.TakeNonce());

  // ReSharper disable CppImplicitDefaultConstructorNotAvailable
  const struct NonceScope {  // NOLINT
    // ReSharper restore CppImplicitDefaultConstructorNotAvailable
    boost::optional<NonceStorage::TakenValue> &nonce;

    ~NonceScope() {
      Assert(nonce);
      nonce = boost::none;
    }
  } nonceScope = {m_nonce};

  const auto result = Base::Send(session);

  Assert(m_nonce);
  m_nonce->Use();

  {
    const auto error = boost::get<1>(result).get_optional<std::string>("error");
    if (error) {
      throw Exception(error->c_str());
    }
  }

  return result;
}

void PrivateRequest::CreateBody(const net::HTTPClientSession &session,
                                std::string &result) const {
  Assert(m_nonce);
  AppendUriParams(m_params, result);
  AppendUriParams("nonce=" + boost::lexical_cast<std::string>(m_nonce->Get()),
                  result);
  Base::CreateBody(session, result);
}

void PrivateRequest::PrepareRequest(const net::HTTPClientSession &session,
                                    const std::string &body,
                                    net::HTTPRequest &request) const {
  request.set("Key", m_settings.apiKey);
  request.set("Sign",
              EncodeToHex(Hmac::CalcSha512Digest(body, m_settings.apiSecret)));
  Base::PrepareRequest(session, body, request);
}