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
#include "RiskControl.hpp"
#include "Settings.hpp"
#include "TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Tests;

namespace lt = boost::local_time;

namespace {
const lt::time_zone_ptr timeZone =
    boost::make_shared<lt::posix_time_zone>("GMT");
Mocks::Context::Log contextLog(timeZone);
Mocks::Context::TradingLog tradingLog(timeZone);
}

Mocks::Context::Context()
    : trdk::Context(contextLog,
                    tradingLog,
                    Settings(),
                    boost::unordered_map<std::string, std::string>()) {}

RiskControl &Mocks::Context::GetRiskControl(const TradingMode &tradingMode) {
  static RiskControl riskControl(
      *this, IniString("[RiskControl]\nis_enabled = no"), tradingMode);
  return riskControl;
}

const RiskControl &Mocks::Context::GetRiskControl(
    const TradingMode &tradingMode) const {
  return const_cast<Mocks::Context *>(this)->GetRiskControl(tradingMode);
}
