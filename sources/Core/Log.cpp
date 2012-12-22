/**************************************************************************
 *   Created: May 20, 2012 6:14:32 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: HighmanTradingRobot
 **************************************************************************/

#include "Prec.hpp"
#include "PyApi/Errors.hpp"

using namespace Trader::Lib;

namespace {

	struct State {

		Log::Mutex mutex;
		volatile long isEnabled;

		std::ostream *log;

		State()
				: log(nullptr) {
			Interlocking::Exchange(isEnabled, false);
		}

		void Enable(std::ostream &newLog) {
			Log::Lock lock(mutex);
			const bool isStarted = !log;
			log = &newLog;
			if (isStarted) {
				AppendRecordHead(boost::get_system_time());
				*log << "Started." << std::endl;
			}
			Interlocking::Exchange(isEnabled, true);
		}

		void Disable() throw() {
			Interlocking::Exchange(isEnabled, false);
		}

		static void AppendRecordHead(
					const boost::posix_time::ptime &time,
					std::ostream &os) {
#			ifdef _WINDOWS
				os << (time + Util::GetEdtDiff()) << " [" << GetCurrentThreadId() << "]: ";
#			else
				os << (time + Util::GetEdtDiff()) << " [" << pthread_self() << "]: ";
#			endif
		}

		void AppendRecordHead(const boost::posix_time::ptime &time) {
			Assert(log);
			AppendRecordHead(time, *log);
		}

	};

	State events;
	State trading;

}

Log::Mutex & Log::GetEventsMutex() {
	return events.mutex;
}

Log::Mutex & Log::GetTradingMutex() {
	return trading.mutex;
}

bool Log::IsEventsEnabled() throw() {
	return events.isEnabled ? true : false;
}

bool Log::IsTradingEnabled() throw() {
	return trading.isEnabled ? true : false;
}

void Log::EnableEvents(std::ostream &log) {
	events.Enable(log);
}

void Log::EnableTrading(std::ostream &log) {
	log.setf(std::ios::left);
	trading.Enable(log);
}

void Log::DisableEvents() throw() {
	events.Disable();
}

void Log::DisableTrading() throw() {
	trading.Disable();
}

void Log::Detail::AppendEventRecordUnsafe(const boost::posix_time::ptime &time, const char *str) {
	Lock lock(events.mutex);
	events.AppendRecordHead(time, std::cout);
	std::cout << str << std::endl;
	if (!events.log) {
		return;
	}
	events.AppendRecordHead(time);
	*events.log << str << std::endl;
}

void Log::Detail::AppendTradingRecordUnsafe(
			const boost::posix_time::ptime &time,
			const char *tag,
			const char *str) {
	Lock lock(trading.mutex);
	if (!trading.log) {
		return;
	}
	trading.AppendRecordHead(time);
	*trading.log << '\t' << tag << '\t' << str << std::endl;
}

void Log::RegisterUnhandledException(
			const char *function,
			const char *file,
			long line,
			bool tradingLog)
		throw() {

	try {

		struct Logger : private boost::noncopyable {

			std::unique_ptr<boost::format> message;
			const bool tradingLog;

			Logger(bool tradingLog)
					: tradingLog(tradingLog) {
				//...//
			}

			boost::format & CreateStandard() {
				return Create(
					"Unhandled %1% exception caught"
						" in function %3%, file %4%, line %5%: \"%2%\".");
			}

			boost::format & Create(const char *format) {
				Assert(!message);
				message.reset(new boost::format(format));
				return *message;
			}

			~Logger() {
				try {
					Assert(message);
					std::cerr << message->str() << std::endl;
					Log::Error(message->str().c_str());
					if (tradingLog) {
						Log::Trading("assert", message->str().c_str());
					}
				} catch (...) {
					AssertFail("Unexpected exception");
				}
			}

		} logger(tradingLog);

		try {
			throw;
		} catch (const Trader::PyApi::ClientError &ex) {
			logger.Create("Py API error, please check your Python script: \"%1%\".") % ex.what();
		} catch (const Trader::Lib::Exception &ex) {
			logger.CreateStandard() % "LOCAL" % ex.what() % function % file % line;
		} catch (const std::exception &ex) {
			logger.CreateStandard() % "STANDART" % ex.what() % function % file % line;
		} catch (...) {
			logger.CreateStandard() % "UNKNOWN" % "" % function % file % line;
		}

	} catch (...) {
		AssertFail("Unexpected exception");
	}

}
