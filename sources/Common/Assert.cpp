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
#include "Assert.hpp"
#include "Core/Log.hpp"
#include "Core/Exception.hpp"

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

void Detatil::RegisterNotExpectedException(
			const char *function,
			const char *file,
			long line) {

	{

		struct Logger {
			
			boost::format message;

			Logger(char const *function, char const *file, long line)
					: message(
						"Unhandled %4% exception caught"
							" in function %1%, file %2%, line %3%: \"%5%\".") {
				message % function % file % line;
			}

			~Logger() {
				std::cerr << message.str() << std::endl;
				Log::Error(message.str().c_str());
				Log::Trading("assert", message.str().c_str());
			}

		} logger(function, file, line);

		try {
			throw;
		} catch (const Exception &ex) {
			logger.message % "LOCAL" % ex.what();
		} catch (const std::exception &ex) {
			logger.message % "STANDART" % ex.what();
		} catch (...) {
			logger.message % "UNKNOWN" % "";
		}

	}

#	if defined(_DEBUG) || defined(_TEST)
		DebugBreak();
#	endif

}
