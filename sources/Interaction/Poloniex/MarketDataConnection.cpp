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
    const boost::unordered_map<std::string, Product> &list,
    const Events &events) {
  if (list.empty()) {
    return;
  }
  ptr::ptree request;
  {
    ptr::ptree products;
    for (const auto &security : list) {
      products.add("command", "subscribe");
      products.add("channel", security.second.id);
    }
    request.add_child("channel", products);
  }
  Handshake("/");
  WebSocketConnection::Start(events);
  Write(request);
}

void MarketDataConnection::Connect() { WebSocketConnection::Connect("https"); }
