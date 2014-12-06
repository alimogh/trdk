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
#include "Api.h"

namespace trdk {

	////////////////////////////////////////////////////////////////////////////////

	//! Log record with delayed formatting.
	class TRDK_CORE_API AsyncLogRecord {

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

			PT_CHAR,
			PT_STRING,
			PT_PCHAR,

			PT_PTIME,
			PT_TIME_DURATION,

			PT_SECURITY,

			numberOfParamTypes
				
		};
		
	public:

		explicit AsyncLogRecord(
				const trdk::Lib::Log::Time &time,
				const trdk::Lib::Log::ThreadId &threadId)
			: m_time(time),
			m_threadId(threadId) {
			//...//
		}

		AsyncLogRecord(AsyncLogRecord &&rhs)
			: m_time(std::move(rhs.m_time)),
			m_threadId(std::move(rhs.m_threadId)),
			m_params(std::move(rhs.m_params)) {
			//...//
		}

	public:

		//! Schedules delayed formatting.
		/** Stores params for log record until it will be really written.
			* WARNING! Pointer to C-string and all other not POD types must be
			* available all time while module is active! Methods stores only
			* pointers, not values.
			* @sa trdk::AsyncLog::AsyncLogRecord::operator %
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
			* @sa trdk::AsyncLog::AsyncLogRecord::Format
			* @sa trdk::AsyncLog::WaitForFlush
			*/
		template<typename Param>
		AsyncLogRecord & operator %(const Param &param) {
			SubFormat(param);
			return *this;
		}

	public:

		const trdk::Lib::Log::Time & GetTime() const {
			return m_time;
		}

		const trdk::Lib::Log::ThreadId & GetThreadId() const {
			return m_threadId;
		}

		const AsyncLogRecord & operator >>(boost::format &os) const {
			Dump(os);
			return *this;
		}

		template<typename Os>
		void Dump(Os &os, const char *delimiter = nullptr) const {

			const auto &begin = m_params.cbegin(); 
			const auto &end = m_params.cend();

			bool skipDelimiter = false;

			for (auto i = begin; i != end; ++i) {

				const ParamType &type = boost::get<0>(*i);
				const boost::any &val = boost::get<1>(*i);

				if (delimiter && i != begin) {
					if (!skipDelimiter) {
						if (
								type != PT_CHAR
								|| (boost::any_cast<char>(val) != '\n'
										&& boost::any_cast<char>(val) != 0)) {
							WriteToDumpStream(delimiter, os);
						} else {
							skipDelimiter = true;
							if (boost::any_cast<char>(val) == 0) {
								continue;
							}
						}
					} else {
						skipDelimiter = false;
					}
				}
	
				static_assert(
					numberOfParamTypes == 16,
					"Parameter type list changed.");
				switch (type) {

					case PT_INT8:
						WriteToDumpStream(boost::any_cast<int8_t>(val), os);
						break;
					case PT_UINT8:
						WriteToDumpStream(boost::any_cast<uint8_t>(val), os);
						break;

					case PT_INT16:
						WriteToDumpStream(boost::any_cast<int16_t>(val), os);
						break;
					case PT_UINT16:
						WriteToDumpStream(boost::any_cast<uint16_t>(val), os);
						break;

					case PT_INT32:
						WriteToDumpStream(boost::any_cast<int32_t>(val), os);
						break;
					case PT_UINT32:
						WriteToDumpStream(boost::any_cast<uint32_t>(val), os);
						break;

					case PT_INT64:
						WriteToDumpStream(boost::any_cast<int64_t>(val), os);
						break;
					case PT_UINT64:
						WriteToDumpStream(boost::any_cast<uint64_t>(val), os);
						break;

					case PT_FLOAT:
						WriteToDumpStream(boost::any_cast<float>(val), os);
						break;
					case PT_DOUBLE:
						WriteToDumpStream(boost::any_cast<double>(val), os);
						break;

					case PT_CHAR:
						WriteToDumpStream(boost::any_cast<char>(val), os);
						break;
					case PT_STRING:
						WriteToDumpStream(
							boost::any_cast<const std::string &>(val),
							os);
						break;
					case PT_PCHAR:
						WriteToDumpStream(
							boost::any_cast<const char *>(val),
							os);
						break;

					case PT_PTIME:
						namespace pt = boost::posix_time;
						WriteToDumpStream(
							boost::any_cast<const pt::ptime &>(val),
							os);
						break;
					case PT_TIME_DURATION:
						namespace pt = boost::posix_time;
						WriteToDumpStream(
							boost::any_cast<const pt::time_duration &>(val),
							os);
						break;

					case PT_SECURITY:
						WriteToDumpStream(
							*boost::any_cast<const Security *>(val),
							os);
						break;

					default:
						AssertEq(PT_UINT64, type);
				}

			}

		}


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

		void StoreParam(char val) {
			StoreTypedParam(PT_CHAR, val);
		}
		void StoreParam(const std::string &val) {
			StoreTypedParam(PT_STRING, val);
		}
		void StoreParam(const char *val) {
			StoreTypedParam(PT_PCHAR, val);
		}

		void StoreParam(const boost::posix_time::ptime &time) {
			StoreTypedParam(PT_PTIME, time);
		}
		void StoreParam(const boost::posix_time::time_duration &time) {
			StoreTypedParam(PT_TIME_DURATION, time);
		}

		void StoreParam(const trdk::Security &security) {
			StoreTypedParam(PT_SECURITY, &security);
		}

		template<typename Param>
		void StoreTypedParam(const ParamType &type, const Param &val) {
			m_params.emplace_back(type, val);
		}

	private:

		template<typename Param>
		static void WriteToDumpStream(const Param &param, boost::format &os) {
			os % param;
		}
		static void WriteToDumpStream(const trdk::Security &, boost::format &);

		template<typename Param>
		static void WriteToDumpStream(const Param &param, std::ostream &os) {
			os << param;
		}
		static void WriteToDumpStream(const trdk::Security &, std::ostream &);

	private:

		trdk::Lib::Log::Time m_time;
		trdk::Lib::Log::ThreadId m_threadId;

		std::vector<boost::tuple<ParamType, boost::any>> m_params;

	};

	////////////////////////////////////////////////////////////////////////////////

	template<trdk::Lib::Concurrency::Profile profile>
	struct AsyncLogConcurrencyPolicy {
		static_assert(
			profile == trdk::Lib::Concurrency::PROFILE_RELAX,
			"Wrong concurrency profile");
		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		typedef boost::condition_variable Condition;
	};

	template<>
	struct AsyncLogConcurrencyPolicy<trdk::Lib::Concurrency::PROFILE_HFT> {
		typedef trdk::Lib::Concurrency::SpinMutex Mutex;
		typedef Mutex::ScopedLock Lock;
		typedef trdk::Lib::Concurrency::SpinCondition Condition;
	};

	////////////////////////////////////////////////////////////////////////////////

	template<
			typename RecordT,
			typename LogT,
			trdk::Lib::Concurrency::Profile concurrencyProfile>
	class AsyncLog : private boost::noncopyable {

	public:

		typedef RecordT Record;
		typedef LogT Log;
		typedef trdk::Lib::Log::Time Time;
		typedef trdk::Lib::Log::ThreadId ThreadId;

	private:

		typedef typename trdk::AsyncLogConcurrencyPolicy<concurrencyProfile>
			ConcurrencyPolicy;
		typedef typename ConcurrencyPolicy::Mutex Mutex;
		typedef typename ConcurrencyPolicy::Lock Lock;
		typedef typename ConcurrencyPolicy::Condition Condition;

		typedef std::vector<Record> Buffer;

		struct Queue {
			Mutex mutex;
			std::pair<Buffer, Buffer> buffers;
			Buffer *activeBuffer;
			Condition condition;
		};

		class WriteTask : private boost::noncopyable {

		public:

			explicit WriteTask(Log &log, Queue &queue)
				:	m_log(log),
					m_queue(queue),
					m_answerCondition(nullptr) {
				Lock lock(m_queue.mutex);
				m_thread = boost::thread([&](){TaskMain();});
				m_queue.condition.wait(lock);
			}

			~WriteTask() {
				try {
					// Signal to stop will be sent from log.
					m_thread.join();
				} catch (...) {
					AssertFailNoException();
				}
			}

		public:
	
			void WaitForFlush() const {
				Lock lock(m_queue.mutex);
				if (!m_queue.activeBuffer || m_queue.activeBuffer->empty()) {
					return;
				}
				Condition answerCondition;
				m_answerCondition = &answerCondition;
				m_queue.condition.notify_one();
				answerCondition.wait(lock);
				Assert(!m_queue.activeBuffer || m_queue.activeBuffer->empty());
				m_answerCondition = nullptr;
			}

		private:

			void TaskMain() {

				try {

					Lock lock(m_queue.mutex);
					m_queue.condition.notify_all();
	
					while (m_queue.activeBuffer) {

						Buffer &buffer = *m_queue.activeBuffer;
						m_queue.activeBuffer
							= m_queue.activeBuffer == &m_queue.buffers.first
								?	&m_queue.buffers.second
								:	&m_queue.buffers.first;

						lock.unlock();
						foreach (const Record &record, buffer) {
							m_log.Write(record);
						}
						buffer.clear();
						lock.lock();

						if (m_queue.activeBuffer) {
							if (!m_queue.activeBuffer->empty()) {
								continue;
							}
							if (m_answerCondition) {
								m_answerCondition->notify_all();
							}
							m_queue.condition.wait(lock);
						}
				
					}

				} catch (...) {
					AssertFailNoException();
					throw;
				}

			}

		private:

			Log &m_log;
			Queue &m_queue;

			boost::thread m_thread;

			mutable Condition *m_answerCondition;

		};

	public:
	
		AsyncLog()
			: m_writeTask(nullptr) {
			m_queue.activeBuffer = &m_queue.buffers.first;
		}

		~AsyncLog() {
			try {
				const Lock lock(m_queue.mutex);
				Assert(m_queue.activeBuffer);
				m_queue.activeBuffer = nullptr;
				m_queue.condition.notify_one();
			} catch (...) {
				AssertFailNoException();
			}
			delete m_writeTask;
		}

	public:

		bool IsEnabled() const {
			return m_log.IsEnabled();
		}

		void EnableStream(std::ostream &os) {
			std::unique_ptr<WriteTask> writeTask;
			if (!m_writeTask) {
				writeTask.reset(new WriteTask(m_log, m_queue));
			}
			m_log.EnableStream(os);
			if (writeTask) {
				m_writeTask = writeTask.release();
			}
			Assert(m_writeTask);
			const Lock lock(m_queue.mutex);
			m_queue.condition.notify_one();
		}
		
	public:

		//! Waits until current queue will be empty.
		/** Needs to be called each time before any DLL will be unload as this
		  * queues may have pointers pointers to memory from this DLL.
		  * @sa trdk::AsyncLog::AsyncLogRecord::Format 
		  * @sa trdk::AsyncLog::AsyncLogRecord::operator %
		  */
		void WaitForFlush() const throw() {
			try {
				m_writeTask->WaitForFlush();
			} catch (...) {
				AssertFailNoException();
			}
		}

	protected:

		template<typename FormatCallback, typename... RecordParams>
		void Write(
				const FormatCallback &formatCallback,
				const RecordParams &...recordParams) {
			if (!IsEnabled()) {
				return;
			}
			Record record(
				m_log.GetTime(),
				m_log.GetThreadId(),
				recordParams...);
			formatCallback(record);
			const Lock lock(m_queue.mutex);
			Assert(m_queue.activeBuffer);
			m_queue.activeBuffer->push_back(std::move(record));
			m_queue.condition.notify_one();
		}

	private:

		Log m_log;
		Queue m_queue;
		WriteTask *m_writeTask;

	};

	////////////////////////////////////////////////////////////////////////////////

}
