//
//    Created: 2018/07/27 10:54 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "Common/VersionInfo.hpp"

using namespace trdk::Lib;

extern "C" TRDK_INTERACTION_HUOBI_API void GetTrdkModuleVersionInfoV1(
    VersionInfoV1 *result) {
  *result = VersionInfoV1(TRDK_INTERACTION_HUOBI_FILE_NAME);
}
