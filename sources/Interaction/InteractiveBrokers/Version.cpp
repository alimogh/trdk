/**************************************************************************
 *   Created: 2016/04/05 07:26:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Api.h"
#include "Common/VersionInfo.hpp"

extern "C" TRDK_INTERACTION_INTERACTIVEBROKERS_API void
GetTrdkModuleVersionInfoV1(trdk::Lib::VersionInfoV1 *result) {
  *result =
      trdk::Lib::VersionInfoV1(TRDK_INTERACTION_INTERACTIVEBROKERS_FILE_NAME);
}
