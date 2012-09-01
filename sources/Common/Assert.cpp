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
#include <Windows.h>
#include "DisableBoostWarningsBegin.h"
#	include <boost/date_time/posix_time/posix_time.hpp>
#	include <boost/thread/thread_time.hpp>
#include "DisableBoostWarningsEnd.h"
#include "Exception.hpp"
#include "Assert.hpp"
#include "Core/Log.hpp"

namespace {

	void Break() {
		DebugBreak();
	}

}

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
			Break();
		}
	}

	namespace Detail {
		void ReportAssertEqFail(
					const std::string &expected,
					const std::string &real,
					const char *expr,
					char const *function,
					const char *file,
					int line) {
			boost::format message(
				"Assertion Failed:"
					" Expecting that values are equal, but it is not: %1% != %2% (which is %3%)"
					" in function %4%, file %5%, line %6%.");
			message % expected % expr % real % function % file % line;
			std::cerr << message.str() << std::endl;
			Log::Error(message.str().c_str());
			Log::Trading("assert", message.str().c_str());
			Break();
		}
		void ReportAssertEqFail(
					const std::string &expected,
					const char *expr,
					char const *function,
					const char *file,
					int line) {
			boost::format message(
				"Assertion Failed:"
					" Expecting that values are NOT equal, but it is: %1% == %2%"
					" in function %3%, file %4%, line %5%.");
			message % expected % expr % function % file % line;
			std::cerr << message.str() << std::endl;
			Log::Error(message.str().c_str());
			Log::Trading("assert", message.str().c_str());
			Break();
		}

		void ReportAssertGtGeLtLeFail(
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
				"Assertion Failed: Expecting that values are %1%, but it is not:"
					" %2% (which is %3%) %4% %5% (which is %6%)"
					" in function %7%, file %8%, line %9%.");
			message % compType % expr1 % val1 % compOp % expr2 % val2 % function % file % line;
			std::cerr << message.str() << std::endl;
			Log::Error(message.str().c_str());
			Log::Trading("assert", message.str().c_str());
			Break();
		}


	}

#else

	namespace Detail {
		
		void ReportAssertFail(const char *expr, const char *file, int line) throw() {
			Log::Error(
				"Assertion Failed (predefined): \"%1%\" in file %2%, line %3%.",
				expr,
				file,
				line);
		}

	}

#endif

void Detail::AssertFailNoExceptionImpl(
			const char *function,
			const char *file,
			long line) {
	Log::RegisterUnhandledException(function, file, line, true);
#	if defined(_DEBUG) || defined(_TEST)
		DebugBreak();
#	endif
}
