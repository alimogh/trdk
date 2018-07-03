//
//    Created: 2018/06/29 11:47
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "GeneratedFiles/Orm/StrategyInstanceRecord-odb.hxx"
#include "GeneratedFiles/Orm/StrategyInstanceViews-odb.hxx"

namespace trdk {
namespace FrontEnd {

typedef odb::query<StrategyInstanceRecord> StrategyInstanceQuery;
typedef odb::query<StrategyInstanceName> StrategyInstanceNameQuery;
}  // namespace FrontEnd
}  // namespace trdk
