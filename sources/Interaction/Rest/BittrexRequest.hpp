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

 public:
  explicit BittrexRequest(const std::string &uri,
                          const std::string &name,
                          const std::string &uriParams)
      : Base("/api/v1.1" + uri,
             name,
             Poco::Net::HTTPRequest::HTTP_GET,
             uriParams) {}
  virtual ~BittrexRequest() override = default;

 public:
  Response Send(Poco::Net::HTTPClientSession &, const Context &) override;

 protected:
  virtual FloodControl &GetFloodControl() override;
};

////////////////////////////////////////////////////////////////////////////////

class BittrexPublicRequest : public BittrexRequest {
 public:
  explicit BittrexPublicRequest(const std::string &name,
                                const std::string &uriParams = std::string())
      : BittrexRequest("/public/" + name, name, uriParams) {}
  virtual ~BittrexPublicRequest() override = default;

 protected:
  virtual bool IsPriority() const override { return false; }
};

////////////////////////////////////////////////////////////////////////////////
}
}
}
