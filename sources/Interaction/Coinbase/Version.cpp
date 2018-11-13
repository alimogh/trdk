/*******************************************************************************
 *   Created: 2018/11/10 12:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Common/VersionInfo.hpp"

using namespace trdk::Lib;

extern "C" __declspec(dllexport) void GetTrdkModuleVersionInfoV1(
    VersionInfoV1 *result) {
  *result = VersionInfoV1(TRDK_INTERACTION_COINBASE_FILE_NAME);
}
