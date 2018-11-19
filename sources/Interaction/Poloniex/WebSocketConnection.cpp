//
//    Created: 2018/11/19 14:53
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "WebSocketConnection.hpp"

using namespace trdk;
using namespace Interaction;
using namespace Poloniex;

WebSocketConnection::WebSocketConnection() : Base("api2.poloniex.com") {}

void WebSocketConnection::Connect() { Base::Connect("https"); }
