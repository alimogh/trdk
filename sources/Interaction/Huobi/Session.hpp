//
//    Created: 2018/07/27 10:36 PM
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
namespace Huobi {

std::unique_ptr<Poco::Net::HTTPSClientSession> CreateSession(
    const Rest::Settings &, bool isTrading);
}
}  // namespace Interaction
}  // namespace trdk