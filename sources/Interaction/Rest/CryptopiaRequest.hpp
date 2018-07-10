/*******************************************************************************
 *   Created: 2017/12/07 15:17:43
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

class CryptopiaRequest : public Request {
 public:
  typedef Request Base;

  explicit CryptopiaRequest(const std::string &name,
                            const std::string &uri,
                            const std::string &method,
                            const std::string &contentType,
                            const Context &context,
                            ModuleEventsLog &log,
                            ModuleTradingLog *tradingLog = nullptr)
      : Base("/api/" + name + "/" + uri,
             name,
             method,
             std::string(),
             context,
             log,
             tradingLog,
             contentType) {}
  ~CryptopiaRequest() override = default;

  Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

 protected:
  FloodControl &GetFloodControl() const override;
};

////////////////////////////////////////////////////////////////////////////////

class CryptopiaPublicRequest : public CryptopiaRequest {
 public:
  explicit CryptopiaPublicRequest(const std::string &name,
                                  const std::string &params,
                                  const Context &context,
                                  ModuleEventsLog &log)
      : CryptopiaRequest(name,
                         params,
                         Poco::Net::HTTPRequest::HTTP_GET,
                         std::string(),
                         context,
                         log) {}
  ~CryptopiaPublicRequest() override = default;

 protected:
  bool IsPriority() const override { return false; }
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
