//
//    Created: 2018/07/08 7:11 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#ifdef ODB_COMPILER
#include "StrategyInstanceRecord.hpp"
#endif

namespace trdk {
namespace FrontEnd {

PRAGMA_DB(view object(StrategyInstanceRecord) query(distinct))
struct StrategyInstanceName {
  QString name;
};

}  // namespace FrontEnd
}  // namespace trdk
