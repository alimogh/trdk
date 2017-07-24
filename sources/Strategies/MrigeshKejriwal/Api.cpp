/*******************************************************************************
 *   Created: 2017/07/22 21:45:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MrigeshKejriwalStrategy.hpp"
#include "Common/VersionInfo.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies;

namespace mk = trdk::Strategies::MrigeshKejriwal;

boost::shared_ptr<Strategy> CreateStrategy(Context &context,
                                           const std::string &instanceName,
                                           const IniSectionRef &conf) {
  return boost::make_shared<mk::Strategy>(context, instanceName, conf);
}

extern "C" void GetTrdkModuleVersionInfoV1(VersionInfoV1 *result) {
  *result = VersionInfoV1(TRDK_STRATEGY_MRIGESHKEJRIWAL_FILE_NAME);
}
