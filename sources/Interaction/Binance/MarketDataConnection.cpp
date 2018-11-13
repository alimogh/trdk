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
using namespace Binance;

MarketDataConnection::MarketDataConnection()
    : WebSocketConnection("stream.binance.com") {}

void MarketDataConnection::Start(
    const boost::unordered_map<ProductId, boost::shared_ptr<Rest::Security>>
        &list,
    const Events &events) {
  if (list.empty()) {
    return;
  }
  std::string request;
  for (const auto &security : list) {
    if (!request.empty()) {
      request += '/';
    }
    request += boost::to_lower_copy(security.first) + "@depth5";
  }
  Handshake("/stream?streams=" + request);
  WebSocketConnection::Start(events);
}

void MarketDataConnection::Connect() { WebSocketConnection::Connect("9443"); }
