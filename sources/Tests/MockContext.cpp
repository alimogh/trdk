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
#include "MockContext.hpp"
#include "Core/RiskControl.hpp"
#include "Core/TradingLog.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Tests;

namespace lt = boost::local_time;

namespace {
	const lt::time_zone_ptr timeZone;
	Context::Log contextLog(timeZone);
	Context::TradingLog tradingLog(timeZone);
}

MockContext::MockContext()
	: Context(
		contextLog,
		tradingLog,
		Settings(),
		boost::unordered_map<std::string, std::string>()) {
	//...//
}

MockContext::~MockContext() {
	//...//
}

RiskControl & MockContext::GetRiskControl(const TradingMode &tradingMode) {
	static RiskControl riskControl(
		*this,
		IniString("[RiskControl]\nis_enabled = no"),
		tradingMode);
	return riskControl;
}

const RiskControl & MockContext::GetRiskControl(
		const TradingMode &tradingMode)
		const {
	return const_cast<MockContext *>(this)->GetRiskControl(tradingMode);
}
