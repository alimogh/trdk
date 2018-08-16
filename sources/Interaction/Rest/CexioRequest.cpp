/*******************************************************************************
 *   Created: 2018/01/18 01:05:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CexioRequest.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction::Rest;
namespace net = Poco::Net;
namespace pt = boost::posix_time;

CexioRequest::CexioRequest(const std::string &name,
                           const std::string &method,
                           const std::string &params,
                           const Context &context,
                           ModuleEventsLog &log,
                           ModuleTradingLog *tradingLog)
    : Base("/api/" + name + "/",
           name,
           method,
           params,
           context,
           log,
           tradingLog) {}

FloodControl &CexioRequest::GetFloodControl() const {
  static auto result = CreateFloodControlWithMaxRequestsPerPeriod(
      600, pt::minutes(10), pt::seconds(2));
  return *result;
}

void CexioRequest::WriteUri(const std::string uri,
                            net::HTTPRequest &request) const {
  const auto &params = GetUriParams();
  if (params.empty() || request.getMethod() == net::HTTPRequest::HTTP_POST) {
    request.setURI(uri);
  } else {
    request.setURI(uri + '?' + params);
  }
}

CexioRequest::Response CexioRequest::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  const auto result = Base::Send(session);
  {
    const auto &response = boost::get<1>(result);
    const auto &errorMessageField = response.get_optional<std::string>("error");
    if (errorMessageField) {
      const auto &errorMessage = *errorMessageField;
      auto isInsufficientFundsError = false;
      auto isCommunicationError = false;
      if (boost::istarts_with(errorMessage, "Rate limit exceeded")) {
        GetLog().Warn("Server rejects requests by rate limit exceeding.");
        GetFloodControl().OnRateLimitExceeded();
        isCommunicationError = true;
      } else {
        isInsufficientFundsError = isCommunicationError =
            boost::icontains(errorMessage, "Insufficient funds");
      }
      std::ostringstream error;
      error << "The server returned an error in response to the request \""
            << GetName() << "\" (" << GetRequest().getURI() << "): "
            << "\"" << errorMessage << "\"";
      isInsufficientFundsError
          ? throw InsufficientFundsException(error.str().c_str())
          : isCommunicationError ? throw CommunicationError(error.str().c_str())
                                 : throw Exception(error.str().c_str());
    }
  }
  return result;
}
