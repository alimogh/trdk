//
//    Created: 2018/11/14 17:06
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace Interaction {
namespace Poloniex {

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

  using Base::Request;
  Request(Request &&) = default;
  Request(const Request &) = delete;
  Request &operator=(Request &&) = delete;
  Request &operator=(const Request &) = delete;
  ~Request() override = default;

 protected:
  Rest::FloodControl &GetFloodControl() const override;
  void CheckErrorResponse(const Poco::Net::HTTPResponse &,
                          const std::string &responseContent,
                          size_t attemptNumber) const override;
};

class PrivateRequest : public Request {
 public:
  typedef Request Base;

  explicit PrivateRequest(const std::string &command,
                          const std::string &params,
                          const Context &,
                          const AuthSettings &,
                          Rest::NonceStorage &,
                          ModuleEventsLog &,
                          ModuleTradingLog &);
  PrivateRequest(PrivateRequest &&) = default;
  PrivateRequest(const PrivateRequest &) = delete;
  PrivateRequest &operator=(PrivateRequest &&) = delete;
  PrivateRequest &operator=(const PrivateRequest &) = delete;
  ~PrivateRequest() override = default;

  Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

 protected:
  bool IsPriority() const override;
  void PrepareRequest(const Poco::Net::HTTPClientSession &,
                      const std::string &body,
                      Poco::Net::HTTPRequest &) const override;
  void CreateBody(const Poco::Net::HTTPClientSession &,
                  std::string &result) const override;

 private:
  const AuthSettings &m_settings;
  Rest::NonceStorage &m_nonces;
  const std::string m_params;
  mutable boost::optional<Rest::NonceStorage::TakenValue> m_nonce;
};

}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk