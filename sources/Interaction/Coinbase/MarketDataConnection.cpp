//
//    Created: 2018/04/07 3:47 PM
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
using namespace Coinbase;
namespace ptr = boost::property_tree;

MarketDataConnection::MarketDataConnection()
    : WebSocketConnection("ws-feed.pro.coinbase.com") {}

void MarketDataConnection::Start(
    const boost::unordered_map<ProductId, SecuritySubscription> &list,
    const Events &events) {
  if (list.empty()) {
    return;
  }

  ptr::ptree request;
  request.add("type", "subscribe");
  {
    ptr::ptree products;
    for (const auto &security : list) {
      products.push_back({"", ptr::ptree().put("", security.first)});
    }
    request.add_child("product_ids", products);
  }
  {
    ptr::ptree channels;
    channels.push_back({"", ptr::ptree().put("", "level2")});
    request.add_child("channels", channels);
  }

  Handshake("/");
  WebSocketConnection::Start(events);
  Write(request);
}

void MarketDataConnection::Connect() { WebSocketConnection::Connect("https"); }
