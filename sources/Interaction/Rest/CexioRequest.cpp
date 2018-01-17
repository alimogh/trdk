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
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace pt = boost::posix_time;

CexioRequest::CexioRequest(const std::string &name,
                           const std::string &method,
                           const std::string &params,
                           const Context &context,
                           ModuleEventsLog &log,
                           ModuleTradingLog *tradingLog)
    : Base("/api/" + name,
           name,
           method,
           std::string(),
           context,
           log,
           tradingLog) {
  SetBody(params);
}

FloodControl &CexioRequest::GetFloodControl() {
  static DisabledFloodControl result;
  return result;
}

void CexioRequest::SetUri(const std::string &uri,
                          net::HTTPRequest &request) const {
  AssertEq(std::string(), GetUriParams());
  request.setURI(uri);
}
