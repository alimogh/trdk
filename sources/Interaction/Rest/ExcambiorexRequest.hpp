/*******************************************************************************
 *   Created: 2018/02/11 18:15:42
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

class ExcambiorexRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit ExcambiorexRequest(const std::string &name,
                              const std::string &method,
                              const std::string &params,
                              const Context &,
                              ModuleEventsLog &,
                              ModuleTradingLog * = nullptr);
  virtual ~ExcambiorexRequest() override = default;

 public:
  virtual Response Send(
      std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

 protected:
  virtual FloodControl &GetFloodControl() const override;
};

////////////////////////////////////////////////////////////////////////////////

class ExcambiorexPublicRequest : public ExcambiorexRequest {
 public:
  explicit ExcambiorexPublicRequest(const std::string &name,
                                    const std::string &params,
                                    const Context &,
                                    ModuleEventsLog &);
  virtual ~ExcambiorexPublicRequest() override = default;

 protected:
  virtual bool IsPriority() const override;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
