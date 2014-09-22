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
#include "Common/AsyncLog.hpp"
#include "Common/LogState.hpp"
#include "Common/LogDetail.hpp"
#include "PyApi/Errors.hpp"

using namespace trdk;
using namespace trdk::Log;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

namespace {

	LogState theEventsLog;

}

////////////////////////////////////////////////////////////////////////////////

namespace {

	struct TradingRecod : public AsyncLogRecord {

		typedef AsyncLogRecord Base;
			
		std::vector<char> tag;
		
		void Save(
					const pt::ptime &time,
					const boost::tuple<
							const std::string &,
							const std::string &> &
						param) {
			Base::Save(time, boost::get<1>(param));
			std::copy(
					boost::get<0>(param).begin(),
					boost::get<0>(param).end(),
					std::back_inserter(this->tag));
			this->tag.push_back(0);
		}

		void Save(
					const pt::ptime &time,
					const boost::tuple<
							const std::string &,
							const char *> &
						param) {
			Base::Save(time, boost::get<1>(param));
			std::copy(
					boost::get<0>(param).begin(),
					boost::get<0>(param).end(),
					std::back_inserter(this->tag));
			this->tag.push_back(0);
		}

		void Flush(trdk::Lib::LogState &log) {
			Assert(log.log);
			log.AppendRecordHead(time);
			*log.log << '\t' << &tag[0] << '\t';
			tag.clear();
			Lib::Detail::DumpMultiLineString(message, *log.log);
			message.clear();
		}

	};

	AsyncLog<TradingRecod> theTradingLog;

}

////////////////////////////////////////////////////////////////////////////////

Log::Mutex & Log::GetEventsMutex() {
	return theEventsLog.mutex;
}

bool Log::IsEventsEnabled(Level /*level*/) throw() {
	return theEventsLog.isStreamEnabled || theEventsLog.isStdOutEnabled
		?	true
		:	false;
}

bool Log::IsTradingEnabled() throw() {
	return theTradingLog.IsEnabled();;
}

void Log::EnableEvents(std::ostream &log) {
	theEventsLog.EnableStream(log);
}

void Log::EnableEventsToStdOut() {
	theEventsLog.EnableStdOut();
}

void Log::EnableTrading(std::ostream &log) {
	log.setf(std::ios::left);
	theTradingLog.EnableStream(log);
}

void Log::DisableEvents() throw() {
	theEventsLog.DisableStream();
}

void Log::DisableEventsToStdOut() throw() {
	theEventsLog.DisableStdOut();
}

void Log::DisableTrading() throw() {
	theTradingLog.DisableStream();
}

//////////////////////////////////////////////////////////////////////////

namespace {
	
	template<typename Str>
	void AppendEventRecordImpl(
				const LogLevel &level,
				const pt::ptime &time,
				const Str &str) {
		Lock lock(theEventsLog.mutex);
		if (theEventsLog.isStdOutEnabled) {
			theEventsLog.AppendRecordHead(level, time, std::cout);
			Lib::Detail::DumpMultiLineString(str, std::cout);
		}
		if (theEventsLog.log) {
 			theEventsLog.AppendRecordHead(level, time);
			Lib::Detail::DumpMultiLineString(str, *theEventsLog.log);
		}
	}

}

void Log::Detail::AppendEventRecordUnsafe(
			const LogLevel &level,
			const pt::ptime &time,
			const char *str) {
	AppendEventRecordImpl(level, time, str);
}

void Log::Detail::AppendEventRecordUnsafe(
			const LogLevel &level,
			const pt::ptime &time,
			const std::string &str) {
	AppendEventRecordImpl(level, time, str);
}

//////////////////////////////////////////////////////////////////////////

void Log::Detail::AppendTradingRecordUnsafe(
			const pt::ptime &time,
			const std::string &tag,
			const char *str) {
	theTradingLog.AppendRecord(time, boost::make_tuple(boost::cref(tag), str));
}

void Log::Detail::AppendTradingRecordUnsafe(
			const pt::ptime &time,
			const std::string &tag,
			const std::string &str) {
	theTradingLog.AppendRecord(
		time,
		boost::make_tuple(boost::cref(tag), boost::cref(str)));
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
			logger.Create(
					"Py API error, please check your Python script: \"%1%\".")
				% ex.what();
		} catch (const Exception &ex) {
			logger.CreateStandard()
				% "LOCAL"
				% ex.what()
				% function
				% file
				% line;
		} catch (const std::exception &ex) {
			logger.CreateStandard()
				% "STANDART"
				% ex.what()
				% function
				% file
				% line;
		} catch (...) {
			logger.CreateStandard() % "UNKNOWN" % "" % function % file % line;
		}

	} catch (...) {
		AssertFail("Unexpected exception");
	}

}
