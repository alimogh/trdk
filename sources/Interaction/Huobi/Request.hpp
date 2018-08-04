//
//    Created: 2018/07/27 7:13 PM
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
namespace Huobi {

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
                          const std::string &method,
                          std::string params,
                          const Context &,
                          const AuthSettings &,
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
  void CreateBody(const Poco::Net::HTTPClientSession &,
                  std::string &result) const override;
  void WriteUri(std::string, Poco::Net::HTTPRequest &) const override;

  std::string m_params;
  const AuthSettings &m_settings;
  const bool m_isPriority;
};

class TradingRequest : public PrivateRequest {
 public:
  explicit TradingRequest(const std::string &name,
                          std::string params,
                          const Context &,
                          const AuthSettings &,
                          ModuleEventsLog &,
                          ModuleTradingLog &);
  TradingRequest(TradingRequest &&) = default;
  TradingRequest(const TradingRequest &) = delete;
  TradingRequest &operator=(TradingRequest &&) = delete;
  TradingRequest &operator=(const TradingRequest &) = delete;
  ~TradingRequest() override = default;
};

}  // namespace Huobi
}  // namespace Interaction
}  // namespace trdk
