/**************************************************************************
 *   Created: May 19, 2012 11:50:26 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#include <assert.h>
#include <boost/format.hpp>
#include <iostream>
#include "DisableBoostWarningsBegin.h"
#	include <boost/date_time/posix_time/posix_time.hpp>
#	include <boost/thread/thread_time.hpp>
#include "DisableBoostWarningsEnd.h"
#include "Exception.hpp"
#ifdef BOOST_WINDOWS
#	include <Windows.h>
#endif
#include "Assert.hpp"
#include "Core/Log.hpp"

#if defined(BOOST_ENABLE_ASSERT_HANDLER)

	namespace boost {
		void assertion_failed(
					char const *expr,
					char const *function,
					char const *file,
					long line) {
			boost::format message("Assertion Failed: \"%1%\" in function %2%, file %3%, line %4%.");
			message % expr % function % file % line;
			std::cerr << message.str() << std::endl;
			Log::Error(message.str().c_str());
			Log::Trading("assert", message.str().c_str());
#			ifdef _WINDOWS
#				if defined(_DEBUG)
					DebugBreak();
#				elif defined(_TEST)
					//...//
#				else
#					error "Failed to find assert-break method."
#				endif
#			else
				__builtin_trap();
				assert(0); // "Execution stopped"
#			endif
		}
	}

#else

	namespace Detatil {
		void ReportAssertFail(const char *expr, const char *file, int line) throw() {
			Log::Error(
				"Assertion Failed (predefined): \"%1%\" in file %2%, line %3%.",
				expr,
				file,
				line);
		}
	}

#endif

void Detatil::AssertFailNoExceptionImpl(
			const char *function,
			const char *file,
			long line) {
	Log::RegisterUnhandledException(function, file, line, true);
#	if defined(_DEBUG) || defined(_TEST)
		DebugBreak();
#	endif
}
