/*******************************************************************************
 *   Created: 2016/02/08 18:50:43
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
  *result = VersionInfoV1(TRDK_STRATEGY_DOCFEELS_FILE_NAME);
}
