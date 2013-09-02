/**************************************************************************
 *   Created: May 20, 2012 6:14:32 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "PyApi/Errors.hpp"

using namespace trdk;
using namespace trdk::Log;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

namespace {

	const std::string debugTag = "Debug";
	const std::string infoTag = "Info";
	const std::string warnTag = "Warn";
	const std::string errorTag = "Error";

	const std::string & GetLevelTag(Level level) {
		static_assert(numberOfLevels == 4, "Log levels list changed.");
		switch (level) {
			case LEVEL_DEBUG:
				return debugTag;
			case LEVEL_INFO:
				return infoTag;
			case LEVEL_WARN:
				return warnTag;
			default:
				AssertFail("Unknown log level.");
			case LEVEL_ERROR:
				return errorTag;
		}
	}

	struct State {

		Log::Mutex mutex;
		volatile long isStreamEnabled;
		volatile long isStdOutEnabled;

		std::ostream *log;

		State()
				: log(nullptr),
				isStreamEnabled(false),
				isStdOutEnabled(false) {
			//...//
		}

		void EnableStream(std::ostream &newLog) {
			Log::Lock lock(mutex);
			const bool isStarted = !log;
			log = &newLog;
			if (isStarted) {
				AppendRecordHead(boost::get_system_time());
				*log << "Started." << std::endl;
			}
			Interlocking::Exchange(isStreamEnabled, true);
		}

		void DisableStream() throw() {
			Interlocking::Exchange(isStreamEnabled, false);
		}

		void EnableStdOut() {
			Interlocking::Exchange(isStdOutEnabled, true);
		}

		void DisableStdOut() throw() {
			Interlocking::Exchange(isStdOutEnabled, false);
		}

		static void AppendRecordHead(const pt::ptime &time, std::ostream &os) {
#			ifdef BOOST_WINDOWS
				os << time << " [" << GetCurrentThreadId() << "]: ";
#			else
				os << time << " [" << pthread_self() << "]: ";
#			endif
		}

		void AppendRecordHead(const pt::ptime &time) {
			Assert(log);
			AppendRecordHead(time, *log);
		}

		void AppendLevelTag(Level level, std::ostream &os) {
			os << std::setw(6) << std::left << GetLevelTag(level);
		}

		void AppendRecordHead(
					Level level,
					const pt::ptime &time,
					std::ostream &os) {
			AppendLevelTag(level, os);
			AppendRecordHead(time, os);
		}

		void AppendRecordHead(Level level, const pt::ptime &time) {
			Assert(log);
			AppendLevelTag(level, *log);
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

bool Log::IsEventsEnabled(Level /*level*/) throw() {
	return events.isStreamEnabled || events.isStdOutEnabled ? true : false;
}

bool Log::IsTradingEnabled() throw() {
	return trading.isStreamEnabled ? true : false;
}

void Log::EnableEvents(std::ostream &log) {
	events.EnableStream(log);
}

void Log::EnableEventsToStdOut() {
	events.EnableStdOut();
}

void Log::EnableTrading(std::ostream &log) {
	log.setf(std::ios::left);
	trading.EnableStream(log);
}

void Log::DisableEvents() throw() {
	events.DisableStream();
}

void Log::DisableEventsToStdOut() throw() {
	events.DisableStdOut();
}

void Log::DisableTrading() throw() {
	trading.DisableStream();
}

////////////////////////////////////////////////////////////////////////////////

namespace {
	
	template<typename Str, typename Stream>
	void DumpMultiLineString(const Str &str, Stream &stream) {
		foreach (char ch, str) {
			stream << ch;
			if (ch == stream.widen('\n')) {
				stream << "\t\t\t\t\t\t";
			}
		}
		stream << std::endl;
	}
}

//////////////////////////////////////////////////////////////////////////

namespace {
	
	template<typename Str>
	void AppendEventRecordImpl(
				Level level,
				const pt::ptime &time,
				const Str &str) {
		Lock lock(events.mutex);
		events.AppendRecordHead(level, time, std::cout);
		DumpMultiLineString(str, std::cout);
		if (!events.log) {
			return;
		}
 		events.AppendRecordHead(level, time);
		DumpMultiLineString(str, *events.log);
	}

}

void Log::Detail::AppendEventRecordUnsafe(
			Level level,
			const pt::ptime &time,
			const char *str) {
	AppendEventRecordImpl(level, time, str);
}

void Log::Detail::AppendEventRecordUnsafe(
			Level level,
			const pt::ptime &time,
			const std::string &str) {
	AppendEventRecordImpl(level, time, str);
}

//////////////////////////////////////////////////////////////////////////

namespace {
	
	template<typename Str>
	void AppendTradingRecordImpl(
				const pt::ptime &time,
				const std::string &tag,
				const Str &str) {
		Lock lock(trading.mutex);
		if (!trading.log) {
			return;
		}
		trading.AppendRecordHead(time);
		*trading.log << '\t' << tag << '\t';
		DumpMultiLineString(str, *trading.log);
	}
}

void Log::Detail::AppendTradingRecordUnsafe(
			const pt::ptime &time,
			const std::string &tag,
			const char *str) {
	AppendTradingRecordImpl(time, tag, str);
}

void Log::Detail::AppendTradingRecordUnsafe(
			const pt::ptime &time,
			const std::string &tag,
			const std::string &str) {
	AppendTradingRecordImpl(time, tag, str);
}

//////////////////////////////////////////////////////////////////////////

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
		} catch (const trdk::PyApi::ClientError &ex) {
			logger.Create("Py API error, please check your Python script: \"%1%\".") % ex.what();
		} catch (const Exception &ex) {
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
