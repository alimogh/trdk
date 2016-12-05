/**************************************************************************
 *   Created: 2016/11/20 20:24:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Common/VersionInfo.hpp"

extern "C" void GetTrdkModuleVersionInfoV1(
		trdk::Lib::VersionInfoV1 *result) {
	*result = trdk::Lib::VersionInfoV1(TRDK_INTERACTION_IQFEED_FILE_NAME);
}
