/*******************************************************************************
 *   Created: 2016/02/08 08:29:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Common/VersionInfo.hpp"
#include "Api.h"

extern "C" TRDK_CORE_API void GetTrdkModuleVersionInfoV1(
		trdk::Lib::VersionInfoV1 *result) {
	*result = trdk::Lib::VersionInfoV1(TRDK_CORE_FILE_NAME);
}
