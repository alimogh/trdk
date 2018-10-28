//
//    Created: 2018/10/27 12:42 AM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "TradingSystemConnection.hpp"

using namespace trdk;
using namespace Interaction;
using namespace Binance;

TradingSystemConnection::TradingSystemConnection()
    : WebSocketConnection("stream.binance.com") {}

void TradingSystemConnection::Start(const std::string &key,
                                    const Events &events,
                                    Context &context) {
  Handshake("/ws/" + key);
  WebSocketConnection::Start(events, context);
}

void TradingSystemConnection ::Connect() {
  WebSocketConnection::Connect("9443");
}
