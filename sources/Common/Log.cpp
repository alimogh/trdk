/**************************************************************************
 *   Created: 2014/11/09 13:46:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Log.hpp"
#include "Util.hpp"

using namespace trdk::Lib;

Log::Log()
		: m_log(nullptr),
		m_isStreamEnabled(false),
		m_isStdOutEnabled(false) {
	//...//
}

Log::~Log() {
	try {
		Write("Start", GetTime(), GetThreadId(), nullptr, "Closed.");
	} catch (...) {
		AssertFailNoException();
	}
}

void Log::EnableStream(std::ostream &newLog, bool writeStartInfo) {
	bool isStarted = false;
	{
		Lock lock(m_streamMutex);
		isStarted = !m_log;
		m_log = &newLog;
		m_isStreamEnabled = true;
	}
	if (isStarted && writeStartInfo) {
		const auto &utc = boost::posix_time::microsec_clock::universal_time();
		Write(
			"Start",
			GetTime(),
			GetThreadId(),
			nullptr,
			boost::format(
					"UTC: \"%1%\". EST: \"%2%\"."
						" Build: \"" TRDK_BUILD_IDENTITY "\"."
						" Build time: \"" __DATE__ " " __TIME__ "\".")
				%	utc
				%	(utc + GetEstDiff()));
	}
}
