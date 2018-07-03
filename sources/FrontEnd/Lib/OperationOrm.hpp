//
//    Created: 2018/06/24 3:52 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "GeneratedFiles/Orm/OperationRecord-odb.hxx"

namespace trdk {
namespace FrontEnd {

typedef odb::query<OperationRecord> OperationQuery;

}  // namespace FrontEnd
}  // namespace trdk
