//
//    Created: 2018/10/14 8:17 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. api.kraken.comPalchukovsky
//

#include "Prec.hpp"
#include "Session.hpp"

using namespace trdk;
using namespace Interaction;
using namespace Rest;
namespace net = Poco::Net;

std::unique_ptr<net::HTTPSClientSession> Kraken::CreateSession(
    const Rest::Settings &settings, const bool isTrading) {
  return Rest::CreateSession("api.kraken.com", settings, isTrading);
}