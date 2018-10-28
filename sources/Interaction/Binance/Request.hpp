//
//    Created: 2018/10/24 08:51
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
namespace Binance {

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

  explicit Request(const std::string &name,
                   const std::string &method,
                   std::string params,
                   const Context &,
                   ModuleEventsLog &,
                   ModuleTradingLog * = nullptr);
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

class PublicRequest : public Request {
 public:
  explicit PublicRequest(const std::string &name,
                         std::string params,
                         const Context &,
                         ModuleEventsLog &);
  PublicRequest(PublicRequest &&) = default;
  PublicRequest(const PublicRequest &) = delete;
  PublicRequest &operator=(PublicRequest &&) = delete;
  PublicRequest &operator=(const PublicRequest &) = delete;
  ~PublicRequest() override = default;

 protected:
  bool IsPriority() const override;
};

class PrivateRequest : public Request {
 public:
  typedef Request Base;

  explicit PrivateRequest(const std::string &name,
                          const std::string &method,
                          const Context &,
                          const AuthSettings &,
                          ModuleEventsLog &,
                          ModuleTradingLog &);
  PrivateRequest(PrivateRequest &&) = default;
  PrivateRequest(const PrivateRequest &) = delete;
  PrivateRequest &operator=(PrivateRequest &&) = delete;
  PrivateRequest &operator=(const PrivateRequest &) = delete;
  ~PrivateRequest() override = default;

 protected:
  bool IsPriority() const override;
  void PrepareRequest(const Poco::Net::HTTPClientSession &,
                      const std::string &body,
                      Poco::Net::HTTPRequest &) const override;

  const AuthSettings &GetSettings() const;

 private:
  std::string m_params;
  const AuthSettings &m_settings;
};

class SignedRequest : public PrivateRequest {
 public:
  typedef PrivateRequest Base;

  explicit SignedRequest(const std::string &name,
                         const std::string &method,
                         std::string params,
                         const Context &,
                         const AuthSettings &,
                         ModuleEventsLog &,
                         ModuleTradingLog &);
  SignedRequest(SignedRequest &&) = default;
  SignedRequest(const SignedRequest &) = delete;
  SignedRequest &operator=(SignedRequest &&) = delete;
  SignedRequest &operator=(const SignedRequest &) = delete;
  ~SignedRequest() override = default;

  Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

 private:
  std::string m_params;
};

}  // namespace Binance
}  // namespace Interaction
}  // namespace trdk
