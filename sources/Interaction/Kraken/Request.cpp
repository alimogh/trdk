//
//    Created: 2018/10/14 1:58 PM
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
using namespace Kraken;
using namespace Crypto;
using namespace Rest;
namespace net = Poco::Net;
namespace ptr = boost::property_tree;

Kraken::Request::Request(const std::string &name,
                         const std::string &method,
                         std::string params,
                         const Context &context,
                         ModuleEventsLog &eventsLog,
                         ModuleTradingLog *tradingLog)
    : Base("/0/" + name,
           name,
           method,
           std::move(params),
           context,
           eventsLog,
           tradingLog) {}

FloodControl &Kraken::Request::GetFloodControl() const {
  static auto result = CreateDisabledFloodControl();
  return *result;
}

Kraken::Request::Response Kraken::Request::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  auto result = Base::Send(session);
  const auto &response = boost::get<1>(result);
  CheckResponseError(response);
  return {boost::get<0>(result), response.get_child("result"),
          boost::get<2>(result)};
}

void Kraken::Request::CheckResponseError(const ptr::ptree &response) const {
  std::vector<std::string> errors;
  for (const auto &error : response.get_child("error")) {
    errors.emplace_back(error.second.get_value<std::string>());
  }
  if (errors.empty()) {
    return;
  }
  boost::format exception(
      "The server returned an error in response to the request \"%1%\" (%2%): "
      "\"%3%\"");
  exception % GetName()             // 1
      % GetRequest().getURI()       // 2
      % boost::join(errors, "; ");  // 3
  errors.size() == 1 && errors.front() == "EOrder:Insufficient funds"
      ? throw InsufficientFundsException(exception.str().c_str())
      : throw Exception(exception.str().c_str());
}

PublicRequest::PublicRequest(const std::string &name,
                             std::string params,
                             const Context &context,
                             ModuleEventsLog &log)
    : Request("public/" + name,
              net::HTTPRequest::HTTP_GET,
              std::move(params),
              context,
              log) {}

bool PublicRequest::IsPriority() const { return false; }

PrivateRequest::PrivateRequest(const std::string &name,
                               std::string params,
                               const Context &context,
                               const AuthSettings &settings,
                               NonceStorage &nonces,
                               const bool isPriority,
                               ModuleEventsLog &log,
                               ModuleTradingLog *tradingLog)
    : Request("private/" + name,
              net::HTTPRequest::HTTP_POST,
              {},
              context,
              log,
              tradingLog),
      m_params(std::move(params)),
      m_settings(settings),
      m_isPriority(isPriority),
      m_nonces(nonces) {}

bool PrivateRequest::IsPriority() const { return m_isPriority; }

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

  return result;
}

void PrivateRequest::PrepareRequest(const net::HTTPClientSession &session,
                                    const std::string &body,
                                    net::HTTPRequest &request) const {
  request.set("API-Key", m_settings.apiKey);
  {
    std::vector<unsigned char> buffer(request.getURI().cbegin(),
                                      request.getURI().cend());
    Assert(m_nonce);
    const auto &postData =
        CalcSha256(boost::lexical_cast<std::string>(m_nonce->Get()) + body);
    buffer.insert(buffer.end(), postData.cbegin(), postData.cend());
    const auto &digest = Hmac::CalcSha512Digest(&buffer[0], buffer.size(),
                                                &m_settings.apiSecret[0],
                                                m_settings.apiSecret.size());
    request.set("API-Sign", Base64::Encode(&digest[0], digest.size(), false));
  }
  Base::PrepareRequest(session, body, request);
}

void PrivateRequest::CreateBody(const net::HTTPClientSession &session,
                                std::string &result) const {
  Assert(m_nonce);
  AppendUriParams("nonce=" + boost::lexical_cast<std::string>(m_nonce->Get()),
                  result);
  AppendUriParams(m_params, result);
  Base::CreateBody(session, result);
}
