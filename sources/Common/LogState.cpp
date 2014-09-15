/**************************************************************************
 *   Created: 2014/09/15 19:52:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "LogState.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace {
	const std::string debugTag = "Debug";
	const std::string infoTag = "Info";
	const std::string warnTag = "Warn";
	const std::string errorTag = "Error";
}

const std::string & Detail::GetLogLevelTag(const LogLevel &level) {
	static_assert(numberOfLogLevels == 4, "Log levels list changed.");
	switch (level) {
		case LOG_LEVEL_DEBUG:
			return debugTag;
		case LOG_LEVEL_INFO:
			return infoTag;
		case LOG_LEVEL_WARN:
			return warnTag;
		default:
			AssertFail("Unknown log level.");
		case LOG_LEVEL_ERROR:
			return errorTag;
	}
}
