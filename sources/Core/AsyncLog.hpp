/**************************************************************************
 *   Created: 2014/11/09 14:19:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"
#include "Common/Log.hpp"
#include "Api.h"

namespace trdk {

	////////////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API AsyncLog : private boost::noncopyable {

	public:

		typedef trdk::Lib::Log::Time Time;
		typedef trdk::Lib::Log::ThreadId ThreadId;

		//! Log record with delayed formatting.
		/** @sa trdk::LogRecord
		  */
		class Record {

		private:

			enum ParamType {

				PT_INT8,
				PT_UINT8,

				PT_INT16,
				PT_UINT16,

				PT_INT32,
				PT_UINT32,

				PT_INT64,
				PT_UINT64,

				PT_FLOAT,
				PT_DOUBLE,

				PT_STRING,
				PT_PCHAR,

				PT_PTIME,

				PT_SECURITY,

				numberOfParamTypes
				
			};
		
		public:

			explicit Record(
						const char *tag,
						const Time &time,
						const ThreadId &threadId,
						const char *message)
					: m_tag(tag),
					m_time(time),
					m_threadId(threadId),
					m_message(message) {
				//...//
			}

			Record(Record &&rhs)
				:	m_tag(rhs.m_tag),
					m_time(rhs.m_time),
					m_threadId(rhs.m_threadId),
					m_message(rhs.m_message),
					m_params(std::move(rhs.m_params)) {
				//...//
			}

		public:

			//! Schedules delayed formatting.
			/** Stores params for log record until it will be really written.
			  * WARNING! Pointer to C-string and all other not POD types must be
			  * available all time while module is active! Methods stores only
			  * pointers, not values.
			  * @sa trdk::AsyncLog::Record::operator %
			  * @sa trdk::AsyncLog::WaitForFlush
			  */
			template<typename... Params>
			void Format(const Params &...params) {
				SubFormat(params...);
			}

			//! Schedules delayed formatting for one parameter.
			/** Stores params for log record until it will be really written.
			  * WARNING! Pointer to C-string and all other not POD types must be
			  * available all time while module is active! Methods stores only
			  * pointers, not values.
			  * @sa trdk::AsyncLog::Record::Format
			  * @sa trdk::AsyncLog::WaitForFlush
			  */
			template<typename Param>
			Record & operator %(const Param &param) {
				SubFormat(param);
				return *this;
			}

		public:

			const char * GetTag() const {
				return m_tag;
			}
			
			const Time & GetTime() const {
				return m_time;
			}
			
			const ThreadId & GetThreadId() const {
				return m_threadId;
			}
			
			std::string GetMessage() const;

		private:

			template<typename FirstParam, typename... OtherParams>
			void SubFormat(
						const FirstParam &firstParam,
						const OtherParams &...otherParams) {
				StoreParam(firstParam);
				SubFormat(otherParams...);
			}

			void SubFormat() {
				//...//
			}

		private:

			void StoreParam(int8_t val) {
				StoreTypedParam(PT_INT8, val);
			}
			void StoreParam(uint8_t val) {
				StoreTypedParam(PT_UINT8, val);
			}

			void StoreParam(int32_t val) {
				StoreTypedParam(PT_INT32, val);
			}
			void StoreParam(uint32_t val) {
				StoreTypedParam(PT_UINT32, val);
			}

			void StoreParam(int64_t val) {
				StoreTypedParam(PT_INT64, val);
			}
			void StoreParam(uint64_t val) {
				StoreTypedParam(PT_UINT64, val);
			}

			void StoreParam(float val) {
				StoreTypedParam(PT_FLOAT, val);
			}
			void StoreParam(double val) {
				StoreTypedParam(PT_DOUBLE, val);
			}

			void StoreParam(const char *val) {
				StoreTypedParam(PT_PCHAR, val);
			}
			void StoreParam(const std::string &val) {
				StoreTypedParam(PT_STRING, val);
			}

			void StoreParam(const boost::posix_time::ptime &time) {
				StoreTypedParam(PT_PTIME, time);
			}

			void StoreParam(const trdk::Security &security) {
				StoreTypedParam(PT_SECURITY, &security);
			}

			template<typename Param>
			void StoreTypedParam(const ParamType &type, const Param &val) {
				m_params.emplace_back(type, val);
			}

		private:

			const char *m_tag;
			Time m_time;
			ThreadId m_threadId;
			const char *m_message;

			std::vector<boost::tuple<ParamType, boost::any>> m_params;

		};

	private:

		template<Lib::Concurrency::Profile profile>
		struct ConcurrencyPolicyT {
			static_assert(
				profile == Lib::Concurrency::PROFILE_RELAX,
				"Wrong concurrency profile");
			typedef boost::mutex Mutex;
			typedef Mutex::scoped_lock Lock;
			typedef boost::condition_variable Condition;
		};
		template<>
		struct ConcurrencyPolicyT<Lib::Concurrency::PROFILE_HFT> {
			typedef Lib::Concurrency::SpinMutex Mutex;
			typedef Mutex::ScopedLock Lock;
			typedef Lib::Concurrency::SpinCondition Condition;
		};
		typedef ConcurrencyPolicyT<TRDK_CONCURRENCY_PROFILE> ConcurrencyPolicy;
		typedef ConcurrencyPolicy::Mutex Mutex;
		typedef ConcurrencyPolicy::Lock Lock;
		typedef ConcurrencyPolicy::Condition Condition;

		typedef std::vector<Record> Buffer;

		struct Queue {
			Mutex mutex;
			std::pair<Buffer, Buffer> buffers;
			Buffer *activeBuffer;
			Condition condition;
		};

	public:
	
		AsyncLog();
		~AsyncLog();

	public:

		bool IsEnabled() const {
			return m_log.IsEnabled();
		}

		void EnableStream(std::ostream &);
		
	public:

		//! Waits until current queue will be empty.
		/** Needs to be called each time before any DLL will be unload as this
		  * queues may have pointers pointers to memory from this DLL.
		  * @sa trdk::AsyncLog::Record::Format 
		  * @sa trdk::AsyncLog::Record::operator %
		  */
		void WaitForFlush() const throw();

	public:

		template<typename FormatCallback>
		void Write(
					const char *tag,
					const char *message,
					const FormatCallback &formatCallback) {
			if (!IsEnabled()) {
				return;
			}
			Record record(tag, m_log.GetTime(), m_log.GetThreadId(), message);
			formatCallback(record);
			const Lock lock(m_queue.mutex);
			Assert(m_queue.activeBuffer);
			m_queue.activeBuffer->push_back(std::move(record));
			m_queue.condition.notify_one();
		}

	private:

		trdk::Lib::Log m_log;

		Queue m_queue;

		class WriteTask;
		WriteTask *m_writeTask;

	};

	typedef trdk::AsyncLog::Record LogRecord;

	////////////////////////////////////////////////////////////////////////////////

	class TRDK_CORE_API ModuleAsyncLog : private boost::noncopyable {

	public:

		explicit ModuleAsyncLog(const std::string &name, trdk::AsyncLog &);

	public:

		void WaitForFlush() const throw() {
			m_log.WaitForFlush();
		}

		template<typename FormatCallback>
		void Write(const char *message, const FormatCallback &formatCallback) {
			m_log.Write(m_namePch, message, formatCallback);
		}

	private:

		const std::string m_name;
		const char *m_namePch;

		trdk::AsyncLog &m_log;

	};

	////////////////////////////////////////////////////////////////////////////////

}
