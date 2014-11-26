/**************************************************************************
 *   Created: 2014/09/15 19:52:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Foreach.hpp"

namespace trdk { namespace Lib {

	class Log : private boost::noncopyable {

	public:

		typedef boost::posix_time::ptime Time;

#		ifdef BOOST_WINDOWS
			typedef DWORD ThreadId;
#		else
			typedef pthread_t ThreadId;
#		endif

	protected:
	
		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

	public:
	
		Log();
		~Log();

	public:

		bool IsEnabled() const {
			return m_isStreamEnabled || m_isStdOutEnabled;
		}

		void EnableStream(std::ostream &);

		void DisableStream() throw() {
			m_isStreamEnabled = false;
		}

		void EnableStdOut() {
			m_isStdOutEnabled = true;
		}

		void DisableStdOut() throw() {
			m_isStdOutEnabled = false;
		}

	public:

		template<typename Message>
		void Write(
				const char *tag,
				const Time &time,
				const ThreadId &theadId,
				const std::string *module,
				const Message &message) {
			if (m_isStreamEnabled) {
				Assert(m_log);
				const Lock lock(m_streamMutex);
				AppendMessage(tag, time, theadId, module, message, *m_log);
			}
			if (m_isStdOutEnabled) {
				const Lock lock(m_stdOutMutex);
				AppendMessage(tag, time, theadId, module, message, std::cout);
			}
		}

		template<typename Message>
		void Write(const Message &message) {
			if (m_isStreamEnabled) {
				Assert(m_log);
				const Lock lock(m_streamMutex);
				AppendMessage(message, *m_log);
			}
			if (m_isStdOutEnabled) {
				const Lock lock(m_stdOutMutex);
				AppendMessage(message, std::cout);
			}
		}

	public:

		Time GetTime() const {
			return std::move(boost::posix_time::microsec_clock::local_time());
		}

		ThreadId GetThreadId() const {
#			ifdef BOOST_WINDOWS
				return GetCurrentThreadId();
#			else
				return pthread_self();
#			endif
		}

	private:

		template<typename Message>
		static void AppendMessage(
				const char *tag,
				const Time &time,
				const ThreadId &theadId,
				const std::string *module,
				const Message &message,
				std::ostream &os) {
			AppendRecordHead(tag, time, theadId, module, os);
			AppendString(message, os);
			AppendRecordEnd(os);
		}

		template<typename Message>
		static void AppendMessage(const Message &message, std::ostream &os) {
			AppendString(message, os);
			AppendRecordEnd(os);
		}

		static void AppendRecordHead(
				const char *tag,
				const boost::posix_time::ptime &time,
				const ThreadId &threadId,
				const std::string *module,
				std::ostream &os) {
			if (tag) {
				os << '[' << tag << "]\t";
			}
			os << time << " [" << threadId << "]: ";
			if (module) {
				os << '[' << *module << "] ";
			}
		}
		static void AppendRecordEnd(std::ostream &os) {
			os << std::endl;
		}

		template<typename Message>
		static void AppendString(
				const Message &message,
				std::ostream &os) {
			os << message;
		}

	private:

		Mutex m_streamMutex;
		boost::atomic_bool m_isStreamEnabled;
		
		Mutex m_stdOutMutex;
		boost::atomic_bool m_isStdOutEnabled;

		std::ostream *m_log;

	};

} }
