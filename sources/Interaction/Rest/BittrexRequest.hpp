/*******************************************************************************
 *   Created: 2017/11/18 13:47:22
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "FloodControl.hpp"
#include "Request.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

////////////////////////////////////////////////////////////////////////////////

class BittrexRequest : public Request {
 public:
  typedef Request Base;

  explicit BittrexRequest(const std::string &uri,
                          const std::string &name,
                          const std::string &uriParams,
                          const Context &context,
                          ModuleEventsLog &log,
                          ModuleTradingLog *tradingLog = nullptr)
      : Base("/api/v1.1" + uri,
             name,
             Poco::Net::HTTPRequest::HTTP_GET,
             uriParams,
             context,
             log,
             tradingLog) {}
  ~BittrexRequest() override = default;

  Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

 protected:
  FloodControl &GetFloodControl() const override;
};

////////////////////////////////////////////////////////////////////////////////

class BittrexPublicRequest : public BittrexRequest {
 public:
  explicit BittrexPublicRequest(const std::string &name,
                                const std::string &uriParams,
                                const Context &context,
                                ModuleEventsLog &log)
      : BittrexRequest("/public/" + name, name, uriParams, context, log) {}
  ~BittrexPublicRequest() override = default;

 protected:
  bool IsPriority() const override { return false; }
};

////////////////////////////////////////////////////////////////////////////////

class BittrexPrivateRequest : public BittrexRequest {
 public:
  typedef BittrexRequest Base;

  explicit BittrexPrivateRequest(const std::string &name,
                                 const std::string &uriParams,
                                 const BittrexSettings &settings,
                                 const Context &context,
                                 ModuleEventsLog &log,
                                 ModuleTradingLog *tradingLog = nullptr);
  ~BittrexPrivateRequest() override = default;

 protected:
  void PrepareRequest(const Poco::Net::HTTPClientSession &,
                      const std::string &,
                      Poco::Net::HTTPRequest &) const override;
  void WriteUri(std::string uri, Poco::Net::HTTPRequest &) const override;

 private:
  const BittrexSettings &m_settings;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
