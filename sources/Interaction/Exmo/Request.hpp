//
//    Created: 2018/04/07 3:47 PM
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
namespace Exmo {

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

  explicit Request(const std::string &name,
                   const std::string &method,
                   const std::string &params,
                   const Context &,
                   ModuleEventsLog &,
                   ModuleTradingLog * = nullptr);
  ~Request() override = default;

 protected:
  Rest::FloodControl &GetFloodControl() const override;
};

class PublicRequest : public Request {
 public:
  explicit PublicRequest(const std::string &name,
                         const std::string &params,
                         const Context &,
                         ModuleEventsLog &);
  ~PublicRequest() override = default;

 protected:
  bool IsPriority() const override;
};

class PrivateRequest : public Request {
 public:
  typedef Request Base;

  explicit PrivateRequest(const std::string &name,
                          const std::string &params,
                          const Context &,
                          const AuthSettings &,
                          Rest::NonceStorage &,
                          bool isPriority,
                          ModuleEventsLog &,
                          ModuleTradingLog * = nullptr);
  ~PrivateRequest() override = default;

  Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

 protected:
  bool IsPriority() const override;

  void CreateBody(const Poco::Net::HTTPClientSession &,
                  std::string &result) const override;
  void PrepareRequest(const Poco::Net::HTTPClientSession &,
                      const std::string &body,
                      Poco::Net::HTTPRequest &) const override;

 private:
  void CheckResponseError(const boost::property_tree::ptree &) const;

  const AuthSettings &m_settings;
  Rest::NonceStorage &m_nonceStorage;
  const bool m_isPriority;
  mutable boost::optional<Rest::NonceStorage::TakenValue> m_nonce;
};

class TradingRequest : public PrivateRequest {
 public:
  explicit TradingRequest(const std::string &name,
                          const std::string &params,
                          const Context &,
                          const AuthSettings &,
                          Rest::NonceStorage &,
                          ModuleEventsLog &,
                          ModuleTradingLog &);
  ~TradingRequest() override = default;
};

}  // namespace Exmo
}  // namespace Interaction
}  // namespace trdk
