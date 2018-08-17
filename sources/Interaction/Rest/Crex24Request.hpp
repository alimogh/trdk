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

  explicit Crex24Request(const std::string &path,
                         const std::string &name,
                         const std::string &method,
                         const std::string &params,
                         const Context &,
                         ModuleEventsLog &,
                         ModuleTradingLog * = nullptr);
  Crex24Request(Crex24Request &&) = default;
  Crex24Request(const Crex24Request &) = delete;
  Crex24Request &operator=(Crex24Request &&) = delete;
  Crex24Request &operator=(const Crex24Request &) = delete;
  ~Crex24Request() override = default;

 protected:
  FloodControl &GetFloodControl() const override;
};

////////////////////////////////////////////////////////////////////////////////

class Crex24PublicRequestV1 : public Crex24Request {
 public:
  explicit Crex24PublicRequestV1(const std::string &name,
                                 const std::string &params,
                                 const Context &,
                                 ModuleEventsLog &);
  Crex24PublicRequestV1(Crex24PublicRequestV1 &&) = default;
  Crex24PublicRequestV1(const Crex24PublicRequestV1 &) = delete;
  Crex24PublicRequestV1 &operator=(Crex24PublicRequestV1 &&) = delete;
  Crex24PublicRequestV1 &operator=(const Crex24PublicRequestV1 &) = delete;
  ~Crex24PublicRequestV1() override = default;

 protected:
  bool IsPriority() const override;
};

class Crex24PublicRequest : public Crex24Request {
 public:
  explicit Crex24PublicRequest(const std::string &name,
                               const std::string &params,
                               const Context &,
                               ModuleEventsLog &);
  Crex24PublicRequest(Crex24PublicRequest &&) = default;
  Crex24PublicRequest(const Crex24PublicRequest &) = delete;
  Crex24PublicRequest &operator=(Crex24PublicRequest &&) = delete;
  Crex24PublicRequest &operator=(const Crex24PublicRequest &) = delete;
  ~Crex24PublicRequest() override = default;

 protected:
  bool IsPriority() const override;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
