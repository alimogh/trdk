/*******************************************************************************
 *   Created: 2017/11/11 13:44:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
namespace net = Poco::Net;

std::unique_ptr<net::HTTPSClientSession> Rest::CreateSession(
    const std::string &host, const Settings &, const bool isTrading) {
  auto result = boost::make_unique<net::HTTPSClientSession>(host);
  result->setKeepAlive(true);
  result->setKeepAliveTimeout(Poco::Timespan(1150, 0));
  result->setTimeout(Poco::Timespan(isTrading ? 60 : 10, 0));
  return result;
}
