//
//    Created: 2018/11/14 16:42
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

std::unique_ptr<Poco::Net::HTTPSClientSession> CreateSession(
    const Rest::Settings &, bool isTrading);
}
}  // namespace Interaction
}  // namespace trdk