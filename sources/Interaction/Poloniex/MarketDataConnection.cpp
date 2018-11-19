//
//    Created: 2018/11/14 16:26
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "MarketDataConnection.hpp"

using namespace trdk;
using namespace Interaction;
using namespace Poloniex;

namespace ptr = boost::property_tree;

MarketDataConnection::MarketDataConnection()
    : WebSocketConnection("api2.poloniex.com") {}

void MarketDataConnection::Start(
    const boost::unordered_map<ProductId, SecuritySubscription> &list,
    const Events &events) {
  if (list.empty()) {
    return;
  }
  Handshake("/");
  WebSocketConnection::Start(events);
  for (const auto &security : list) {
    ptr::ptree request;
    request.add("command", "subscribe");
    request.add("channel", security.first);
    Write(request);
  }
}

void MarketDataConnection::Connect() { WebSocketConnection::Connect("https"); }
