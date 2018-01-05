/*******************************************************************************
 *   Created: 2017/12/12 10:20:20
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

class LivecoinRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit LivecoinRequest(const std::string &name,
                           const std::string &method,
                           const std::string &params,
                           const Context &context,
                           ModuleEventsLog &log,
                           ModuleTradingLog *tradingLog = nullptr)
      : Base(name, name, method, params, context, log, tradingLog) {}
  virtual ~LivecoinRequest() override = default;

 protected:
  virtual FloodControl &GetFloodControl() override;
};

////////////////////////////////////////////////////////////////////////////////

class LivecoinPublicRequest : public LivecoinRequest {
 public:
  explicit LivecoinPublicRequest(const std::string &name,
                                 const std::string &params,
                                 const Context &context,
                                 ModuleEventsLog &log)
      : LivecoinRequest(
            name, Poco::Net::HTTPRequest::HTTP_GET, params, context, log) {}
  virtual ~LivecoinPublicRequest() override = default;

 protected:
  virtual bool IsPriority() const override { return false; }
};

////////////////////////////////////////////////////////////////////////////////
}
}
}
