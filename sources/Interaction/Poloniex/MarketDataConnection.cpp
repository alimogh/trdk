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

MarketDataConnection::MarketDataConnection(
    const boost::unordered_map<ProductId, SecuritySubscription> &subscription)
    : m_subscription(subscription) {}

void MarketDataConnection::StartData(const Events &events) {
  if (m_subscription.empty()) {
    return;
  }
  Handshake("/");
  Start(events);
  for (const auto &security : m_subscription) {
    ptr::ptree request;
    request.add("command", "subscribe");
    request.add("channel", security.first);
    Write(request);
  }
}
