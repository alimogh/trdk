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
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

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

FloodControl &CexioRequest::GetFloodControl() {
  static DisabledFloodControl result;
  return result;
}

void CexioRequest::SetUri(const std::string &uri,
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
    const auto &errorMessage = response.get_optional<std::string>("error");
    if (errorMessage) {
      std::ostringstream error;
      error << "The server returned an error in response to the request \""
            << GetName() << "\" (" << GetRequest().getURI() << "): "
            << "\"" << *errorMessage << "\"";
      throw Exception(error.str().c_str());
    }
  }
  return result;
}
