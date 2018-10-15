//
//    Created: 2018/10/11 08:31
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
namespace Kraken {

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

  Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

 protected:
  Rest::FloodControl &GetFloodControl() const override;
  void CheckResponseError(const boost::property_tree::ptree &) const;
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
                          std::string params,
                          const Context &,
                          const AuthSettings &,
                          Rest::NonceStorage &,
                          bool isPriority,
                          ModuleEventsLog &,
                          ModuleTradingLog * = nullptr);
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

  std::string m_params;
  const AuthSettings &m_settings;
  const bool m_isPriority;
  Rest::NonceStorage &m_nonces;
  mutable boost::optional<Rest::NonceStorage::TakenValue> m_nonce;
};

}  // namespace Kraken
}  // namespace Interaction
}  // namespace trdk
