//
//    Created: 2018/11/14 17:06
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
namespace Poloniex {

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

 protected:
  Rest::FloodControl &GetFloodControl() const override;
  void CheckErrorResponse(const Poco::Net::HTTPResponse &,
                          const std::string &responseContent,
                          size_t attemptNumber) const override;
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

}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk