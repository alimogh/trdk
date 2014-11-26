/**************************************************************************
 *   Created: 2014/11/24 23:13:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "FxMbStrategyLog.hpp"
#include "Core/Strategy.hpp"
#include "Core/Context.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;

////////////////////////////////////////////////////////////////////////////////

StrategyLog::StrategyLog()
	: m_opportunityNumber(1) {
	//...//
}

StrategyLog::~StrategyLog() {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

StrategyLog & trdk::Strategies::FxMb::GetStrategyLog(const Strategy &strategy) {

	static std::ofstream file;
	static StrategyLog instance;

	const auto &logFilePath
		= strategy.GetContext().GetSettings().GetLogsDir() / "strategy.log";
	
	if (!instance.IsEnabled()) {
		boost::filesystem::create_directories(logFilePath.branch_path());
		file.open(logFilePath.string().c_str());
		if (!file) {
			strategy.GetLog().Error(
				"Failed to open strategy log %1%.",
				logFilePath);
			throw SystemException("Failed to open strategy log");
		}
		instance.EnableStream(file);
		Assert(instance.IsEnabled());
	}
	
	strategy.GetLog().Info("Strategy log: %1%.", logFilePath);

	return instance;

}

////////////////////////////////////////////////////////////////////////////////
