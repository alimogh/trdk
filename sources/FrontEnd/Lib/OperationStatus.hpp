//
//    Created: 2018/06/24 10:12 AM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

namespace trdk {
namespace FrontEnd {

BETTER_ENUM(OperationStatus,
            std::uint8_t,
            Active = 10,
            Canceled = 21,
            Completed = 22,
            Profit = 23,
            Loss = 24,
            Error = 30);

}  // namespace FrontEnd
}  // namespace trdk
