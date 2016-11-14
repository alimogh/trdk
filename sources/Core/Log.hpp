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

#include "Api.h"

namespace trdk {

	class TRDK_CORE_API Log : private boost::noncopyable {

	public:

#		ifdef BOOST_WINDOWS
			typedef DWORD ThreadId;
#		else
			typedef pthread_t ThreadId;
#		endif

	private:
	
		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

	public:
	
		explicit Log(const boost::local_time::time_zone_ptr &);
		~Log();

	public:

		bool IsEnabled() const noexcept {
			return m_isStreamEnabled || m_isStdOutEnabled;
		}

		void EnableStream(std::ostream &, bool writeStartInfo);

		void DisableStream() noexcept {
			m_isStreamEnabled = false;
		}

		void EnableStdOut() {
			m_isStdOutEnabled = true;
		}

		void DisableStdOut() noexcept {
			m_isStdOutEnabled = false;
		}

	public:

		template<typename Message>
		void Write(
				const char *tag,
				const boost::posix_time::ptime &time,
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

		boost::posix_time::ptime GetTime() const {
			namespace lt = boost::local_time;
			return lt::local_microsec_clock::local_time(GetTimeZone())
				.local_time();
		}

		ThreadId GetThreadId() const {
#			ifdef BOOST_WINDOWS
				return GetCurrentThreadId();
#			else
				return pthread_self();
#			endif
		}

		const boost::local_time::time_zone_ptr & GetTimeZone() const {
			return m_timeZone;
		}

	private:

		template<typename Message>
		static void AppendMessage(
				const char *tag,
				const boost::posix_time::ptime &time,
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
				const boost::posix_time::ptime &,
				const ThreadId &,
				const std::string *module,
				std::ostream &);
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

		const boost::local_time::time_zone_ptr m_timeZone;

		Mutex m_streamMutex;
		boost::atomic_bool m_isStreamEnabled;
		
		Mutex m_stdOutMutex;
		boost::atomic_bool m_isStdOutEnabled;

		std::ostream *m_log;

	};

}
