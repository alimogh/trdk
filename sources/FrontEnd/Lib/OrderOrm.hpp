//
//    Created: 2018/06/29 11:49
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "GeneratedFiles/Orm/OrderRecord-odb.hxx"

namespace trdk {
namespace FrontEnd {

typedef odb::query<OrderRecord> OrderQuery;

}  // namespace FrontEnd
}  // namespace trdk
