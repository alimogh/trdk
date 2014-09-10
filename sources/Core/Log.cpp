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
		boost::atomic_bool isStreamEnabled;
		boost::atomic_bool isStdOutEnabled;

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
			isStreamEnabled = true;
		}

		void DisableStream() throw() {
			isStreamEnabled = false;
		}

		void EnableStdOut() {
			isStdOutEnabled = true;
		}

		void DisableStdOut() throw() {
			isStdOutEnabled = false;
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

	State theEventsLog;

}

////////////////////////////////////////////////////////////////////////////////

namespace {

	template<typename Str, typename Stream>
	void DumpMultiLineString(const Str &str, Stream &stream) {
		foreach (char ch, str) {
			if (ch == 0) {
				break;
			}
			stream << ch;
			if (ch == stream.widen('\n')) {
				stream << "\t\t\t\t\t\t";
			}
		}
		stream << std::endl;
	}

}

////////////////////////////////////////////////////////////////////////////////

namespace {

	class TradingLog : private boost::noncopyable {

	private:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		typedef boost::condition_variable Condition;

		struct Record {
			
			pt::ptime time;
			std::vector<char> tag;
			std::vector<char> message;
		
			void Save(
						const pt::ptime &time,
						const std::string &tag,
						const std::string &message) {
				this->time = time;
				std::copy(
					tag.begin(),
					tag.end(),
					std::back_inserter(this->tag));
				this->tag.push_back(0);
				std::copy(
					message.begin(),
					message.end(),
					std::back_inserter(this->message));
				this->message.push_back(0);
			}

			void Save(
						const pt::ptime &time,
						const std::string &tag,
						const char *message) {
				this->time = time;
				std::copy(
					tag.begin(),
					tag.end(),
					std::back_inserter(this->tag));
				this->tag.push_back(0);
				std::copy(
					message,
					message + strlen(message) + 1,
					std::back_inserter(this->message));
			}

		};

		struct Buffer {
			
			std::vector<Record> records;
			std::vector<Record>::iterator end;
		
			Buffer()
					: end(records.begin()) {
				//...//
			}
		
		};

	public:

		TradingLog()
				: m_currentBuffer(&m_buffers.first) {
			//...//
		}

		~TradingLog() {
			DisableStream();
		}

	public:

		bool IsEnabled() const throw() {
			return m_log.isStreamEnabled;
		}

		void EnableStream(std::ostream &newLog) {
			Lock lock(m_mutex);
			Assert(!m_writeThread);
			while (m_writeThread) {
				lock.release();
				DisableStream();
				lock.lock();
			}
			m_log.EnableStream(newLog);
			m_writeThread.reset(new boost::thread(
				[&]() {
					try {
						WriteTask();
					} catch (...) {
						AssertFailNoException();
						throw;
					}
				}));
			m_condition.wait(lock);
		}

		void DisableStream() throw() {
			try {
				Lock lock(m_mutex);
				m_log.DisableStream();
				if (!m_writeThread) {
					return;
				}
				auto thread = m_writeThread;
				m_writeThread.reset();
				lock.release();
				m_condition.notify_all();
				thread->join();
			} catch (...) {
				AssertFailNoException();
			}
		}

	public:

		template<typename Str>
		void AppendRecord(
					const pt::ptime &time,
					const std::string &tag,
					const Str &str) {
			{
				const Lock lock(m_mutex);
				if (m_currentBuffer->end == m_currentBuffer->records.end()) {
					m_currentBuffer->records.emplace(
						m_currentBuffer->records.end());
					m_currentBuffer->end = m_currentBuffer->records.end();
					m_currentBuffer->records.back().Save(time, tag, str);
				} else {
					m_currentBuffer->end->Save(time, tag, str);
					++m_currentBuffer->end;
				}
			}
			m_condition.notify_one();
		}

	private:

		void WriteTask() {

			m_condition.notify_all();
		
			Lock lock(m_mutex);
		
			while (m_writeThread) {

				Buffer *currentBuffer = m_currentBuffer;
				m_currentBuffer = m_currentBuffer == &m_buffers.first
					?	&m_buffers.second
					:	&m_buffers.first;

				lock.unlock();
				{
					for (
							auto record = currentBuffer->records.begin();
							record != currentBuffer->end;
							++record) {
						m_log.AppendRecordHead(record->time);
						*m_log.log << '\t' << &record->tag[0] << '\t';
						DumpMultiLineString(record->message, *m_log.log);
						record->tag.empty();
						record->message.empty();
					}
					currentBuffer->end = currentBuffer->records.begin();
				}
				lock.lock();

				if (m_currentBuffer->records.begin() != m_currentBuffer->end) {
					continue;
				}
				
				m_condition.wait(lock);
		
			}
		
		}


	private:

		std::pair<Buffer, Buffer> m_buffers;
		Buffer *m_currentBuffer;

		State m_log;

		Mutex m_mutex;
		Condition m_condition;
		boost::shared_ptr<boost::thread> m_writeThread;

	} theTradingLog;

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
				Level level,
				const pt::ptime &time,
				const Str &str) {
		Lock lock(theEventsLog.mutex);
		if (theEventsLog.isStdOutEnabled) {
			theEventsLog.AppendRecordHead(level, time, std::cout);
			DumpMultiLineString(str, std::cout);
		}
		if (theEventsLog.log) {
 			theEventsLog.AppendRecordHead(level, time);
			DumpMultiLineString(str, *theEventsLog.log);
		}
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

void Log::Detail::AppendTradingRecordUnsafe(
			const pt::ptime &time,
			const std::string &tag,
			const char *str) {
	theTradingLog.AppendRecord(time, tag, str);
}

void Log::Detail::AppendTradingRecordUnsafe(
			const pt::ptime &time,
			const std::string &tag,
			const std::string &str) {
	theTradingLog.AppendRecord(time, tag, str);
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
