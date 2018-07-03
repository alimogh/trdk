/*******************************************************************************
 *   Created: 2016/02/07 03:30:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "ContextMock.hpp"
#include "Core/Settings.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace Lib;
using namespace Tests;
using namespace Core;
namespace lt = boost::local_time;
namespace ptr = boost::property_tree;

namespace {
const lt::time_zone_ptr timeZone =
    boost::make_shared<lt::posix_time_zone>("GMT");
ContextMock::Log contextLog(timeZone);
ContextMock::TradingLog tradingLog(timeZone);
}  // namespace

ContextMock::ContextMock() : Context(contextLog, tradingLog, Settings()) {}

RiskControl &ContextMock::GetRiskControl(const TradingMode &tradingMode) {
  static RiskControl riskControl(
      *this,
      ptr::ptree().add_child("RiskControl",
                             ptr::ptree().add("isEnabled", false)),
      tradingMode);
  return riskControl;
}

const RiskControl &ContextMock::GetRiskControl(
    const TradingMode &tradingMode) const {
  return const_cast<ContextMock *>(this)->GetRiskControl(tradingMode);
}
