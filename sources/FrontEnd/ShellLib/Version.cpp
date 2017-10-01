/*******************************************************************************
 *   Created: 2017/10/01 19:17:21
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

extern "C" void GetTrdkModuleVersionInfoV1(VersionInfoV1 *result) {
  *result = VersionInfoV1(TRDK_FRONTEND_SHELL_LIB_FILE_NAME);
}
