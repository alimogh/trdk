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
#include "Core/Context.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::FxMb;

////////////////////////////////////////////////////////////////////////////////

StrategyLog::StrategyLog() {
	//...//
}

StrategyLog::~StrategyLog() {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

StrategyLog & trdk::Strategies::FxMb::GetStrategyLog(const Context &context) {

	static std::ofstream file;
	static StrategyLog instance;
	
	if (!instance.IsEnabled()) {
		const auto &logFilePath
			= context.GetSettings().GetLogsDir() / "strategy.log";
		boost::filesystem::create_directories(logFilePath.branch_path());
		file.open(logFilePath.string().c_str());
		if (!file) {
			context.GetLog().Error(
				"Failed to open strategy log %1%.",
				logFilePath);
			throw SystemException("Failed to open strategy log");
		}
		instance.EnableStream(file);
		Assert(instance.IsEnabled());
	}

	return instance;

}

////////////////////////////////////////////////////////////////////////////////
