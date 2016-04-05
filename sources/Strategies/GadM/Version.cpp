/**************************************************************************
 *   Created: 2016/04/05 08:23:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Common/VersionInfo.hpp"

extern "C" TRDK_STRATEGY_GADM_API void GetTrdkModuleVersionInfoV1(
		trdk::Lib::VersionInfoV1 *result) {
	*result = trdk::Lib::VersionInfoV1(TRDK_STRATEGY_GADM_FILE_NAME);
}
