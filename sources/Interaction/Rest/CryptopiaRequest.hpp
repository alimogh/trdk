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

 public:
  explicit CryptopiaRequest(const std::string &name,
                            const std::string &uri,
                            const std::string &method,
                            const std::string &contentType = std::string())
      : Base("/api/" + name + "/" + uri,
             name,
             method,
             std::string(),
             contentType) {}
  virtual ~CryptopiaRequest() override = default;

 public:
  Response Send(Poco::Net::HTTPClientSession &, const Context &) override;

 protected:
  virtual FloodControl &GetFloodControl() override;
};

////////////////////////////////////////////////////////////////////////////////

class CryptopiaPublicRequest : public CryptopiaRequest {
 public:
  explicit CryptopiaPublicRequest(const std::string &name,
                                  const std::string &params = std::string())
      : CryptopiaRequest(name, params, Poco::Net::HTTPRequest::HTTP_GET) {}
  virtual ~CryptopiaPublicRequest() override = default;

 protected:
  virtual bool IsPriority() const override { return false; }
};

////////////////////////////////////////////////////////////////////////////////
}
}
}
