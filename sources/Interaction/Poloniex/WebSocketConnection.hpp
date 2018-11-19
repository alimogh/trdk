//
//    Created: 2018/11/19 14:52
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace Interaction {
namespace Poloniex {

class WebSocketConnection : public Lib::WebSocketConnection {
 public:
  typedef Lib::WebSocketConnection Base;

  WebSocketConnection();
  void Connect();
};
}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk
