/*******************************************************************************
 *   Created: 2018/02/16 04:17:45
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

class Crex24Request : public Request {
 public:
  typedef Request Base;

 public:
  explicit Crex24Request(const std::string &path,
                         const std::string &name,
                         const std::string &method,
                         const std::string &params,
                         const Context &,
                         ModuleEventsLog &,
                         ModuleTradingLog * = nullptr);
  virtual ~Crex24Request() override = default;

 protected:
  virtual FloodControl &GetFloodControl() const override;
};

////////////////////////////////////////////////////////////////////////////////

class Crex24PublicRequest : public Crex24Request {
 public:
  explicit Crex24PublicRequest(const std::string &name,
                               const std::string &params,
                               const Context &,
                               ModuleEventsLog &);
  virtual ~Crex24PublicRequest() override = default;

 protected:
  virtual bool IsPriority() const override;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
