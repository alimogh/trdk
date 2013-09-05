/**************************************************************************
 *   Created: May 19, 2012 11:50:26 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Util.hpp"
#include "Core/Log.hpp"
#include "Assert.hpp"
#include "Common/Constants.h"

using namespace trdk;
using namespace trdk::Debug;

#if defined(_DEBUG)
#	define IS_DEBUG_BREAK_ENABLED 1
#elif defined(DEV_VER)
#	define IS_DEBUG_BREAK_ENABLED 1
#else
#	define IS_DEBUG_BREAK_ENABLED 0
#endif

namespace {

	void BreakDebug() {
#		if IS_DEBUG_BREAK_ENABLED == 1
#			if defined(BOOST_WINDOWS)
				DebugBreak();
#			elif defined(_DEBUG)
				__assert_fail("Debug break", "", 0, "");
#			endif
#		else
		{
			const char *message
				= "An unexpected program failure occurred! Please send this"
					" and other product log files at " TRDK_SUPPORT_EMAIL
					" with descriptions of actions that you have made during"
					" the occurrence of the failure.";
			Log::Error(message);
			Log::Trading("error", message);
		}
#		endif
	}

}

#if defined(BOOST_ENABLE_ASSERT_HANDLER)

	namespace boost {
		void assertion_failed(
					char const *expr,
					char const *function,
					char const *file,
					long line) {
			boost::format message(
				"Assertion Failed:"
					" \"%1%\" in function %2%, file %3%, line %4%.");
			message % expr % function % file % line;
			std::cerr << message.str() << std::endl;
			Log::Error(message.str().c_str());
			Log::Trading("assert", message.str().c_str());
			BreakDebug();
		}
	}

	void Detail::ReportCompareAssertFail(
				const std::string &val1,
				const std::string &val2,
				const char *compType,
				const char *compOp,
				const char *expr1,
				const char *expr2,
				const char *function,
				const char *file,
				int line) {
		boost::format message(
			"Assertion Failed: Expecting that value %1%, but it is not:"
				" %2% (which is %3%) %4% %5% (which is %6%)"
				" in function %7%, file %8%, line %9%.");
		message
			% compType
			% expr1
			% val1
			% compOp
			% expr2
			% val2
			% function
			% file
			% line;
		std::cerr << message.str() << std::endl;
		Log::Error(message.str().c_str());
		Log::Trading("assert", message.str().c_str());
		BreakDebug();
	}

#endif

void Detail::ReportAssertFail(
			const char *expr,
			const char *file,
			int line)
		throw() {
	Log::Error(
		"Assertion Failed (predefined): \"%1%\" in file %2%, line %3%.",
		expr,
		file,
		line);
	BreakDebug();
}

void Detail::RegisterUnhandledException(
			const char *function,
			const char *file,
			long line,
			bool tradingLog) {
	Log::RegisterUnhandledException(function, file, line, tradingLog);
}

void Detail::AssertFailNoExceptionImpl(
			const char *function,
			const char *file,
			long line) {
	RegisterUnhandledException(function, file, line, true);
	BreakDebug();
}
