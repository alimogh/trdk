/*******************************************************************************
 *   Created: 2017/08/20 14:22:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DocFeelsCumulativeReturnFilterService.hpp"
#include "DocFeelsCumulativeReturnService.hpp"
#include "DocFeelsStrategy.hpp"
#include "Common/VersionInfo.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies;

namespace df = trdk::Strategies::DocFeels;

boost::shared_ptr<Strategy> CreateTestStrategy(Context &context,
                                               const std::string &instanceName,
                                               const IniSectionRef &conf) {
  return boost::make_shared<df::Strategy>(context, instanceName, conf);
}

boost::shared_ptr<Service> CreateCumulativeReturnService(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<df::CumulativeReturnService>(context, instanceName,
                                                         configuration);
}
boost::shared_ptr<Service> CreateCumulativeReturnFilterService(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<df::CumulativeReturnFilterService>(
      context, instanceName, configuration);
}

extern "C" void GetTrdkModuleVersionInfoV1(VersionInfoV1 *result) {
  *result = VersionInfoV1(TRDK_STRATEGY_DOCFEELS_FILE_NAME);
}
