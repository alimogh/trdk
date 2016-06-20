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

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

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
		terminate();
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
		const auto &utc = pt::microsec_clock::universal_time();
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

void Log::AppendRecordHead(
		const char *tag,
		const boost::posix_time::ptime &time,
		const ThreadId &threadId,
		const std::string *module,
		std::ostream &os) {
	// The method should be in this obj-file to use one
	// boost::date_time::time_facet for all modules or each module will create
	// own and it will crash program after DLL unloading.
	if (tag) {
		os << '[' << tag << "]\t";
	}
	os << time;
#			ifdef BOOST_WINDOWS
	os << "\t[";
#			else
	os << "\t[0x" << std::ios::hex;
#			endif
	os << threadId << "]:\t";
	if (module) {
		os << '[' << *module << "] ";
	}
}
