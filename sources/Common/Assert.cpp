/**************************************************************************
 *   Created: May 19, 2012 11:50:26 AM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#include <assert.h>

#if defined(BOOST_ENABLE_ASSERT_HANDLER)

#	include "Assert.hpp"
#	include <boost/format.hpp>
#	include <iostream>
#	include <Windows.h>
#	include <boost/date_time/posix_time/posix_time.hpp>
#	include "Core/Log.hpp"

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
#				ifdef _DEBUG
					_wassert(L"Execution stopped", _CRT_WIDE(__FILE__), __LINE__);
#				elif defined(_TEST)
					DebugBreak();
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
