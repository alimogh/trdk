//
//    Created: 2018/11/14 16:43
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "Session.hpp"

using namespace trdk;
using namespace Interaction;
using namespace Rest;
namespace net = Poco::Net;

std::unique_ptr<net::HTTPSClientSession> Poloniex::CreateSession(
    const Rest::Settings &settings, const bool isTrading) {
  return Rest::CreateSession("poloniex.com", settings, isTrading);
}