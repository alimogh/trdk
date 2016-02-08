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
#include "Api.h"

extern "C" TRDK_INTERACTION_FAKE_API void GetTrdkModuleVersionInfoV1(
		trdk::Lib::VersionInfoV1 *result) {
	*result = trdk::Lib::VersionInfoV1(TRDK_INTERACTION_FAKE_FILE_NAME);
}
