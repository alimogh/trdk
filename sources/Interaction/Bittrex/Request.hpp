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

namespace trdk {
namespace Interaction {
namespace Bittrex {

////////////////////////////////////////////////////////////////////////////////

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

  explicit Request(const std::string &uri,
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
  ~Request() override = default;

  Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

 protected:
  Rest::FloodControl &GetFloodControl() const override;
};

////////////////////////////////////////////////////////////////////////////////

class PublicRequest : public Request {
 public:
  explicit PublicRequest(const std::string &name,
                         const std::string &uriParams,
                         const Context &context,
                         ModuleEventsLog &log)
      : Request("/public/" + name, name, uriParams, context, log) {}
  ~PublicRequest() override = default;

 protected:
  bool IsPriority() const override { return false; }
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace Bittrex
}  // namespace Interaction
}  // namespace trdk
