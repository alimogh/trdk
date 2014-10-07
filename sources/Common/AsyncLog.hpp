/**************************************************************************
 *   Created: 2014/09/15 19:35:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "LogState.hpp"
#include "LogDetail.hpp"
#include "DisableBoostWarningsBegin.h"
#	include <boost/noncopyable.hpp>
#	include <boost/thread/mutex.hpp>
#	include <boost/thread/condition.hpp>
#	include <boost/thread/thread.hpp>
#	include <boost/date_time/posix_time/ptime.hpp>
#	include <boost/shared_ptr.hpp>
#include "DisableBoostWarningsEnd.h"
#include <vector>

namespace trdk { namespace Lib {

	////////////////////////////////////////////////////////////////////////////////

	struct AsyncLogRecord {
		
		typedef std::vector<char> String;
		
		boost::posix_time::ptime time;
		String message;

		void Save(
					const boost::posix_time::ptime &time,
					const std::string &message) {
			this->time = time;
			std::copy(
				message.begin(),
				message.end(),
				std::back_inserter(this->message));
			this->message.push_back(0);
		}

		void Save(
					const boost::posix_time::ptime &time,
					const char *message) {
			this->time = time;
			std::copy(
				message,
				message + strlen(message) + 1,
				std::back_inserter(this->message));
		}

		void Flush(trdk::Lib::LogState &log) {
			Assert(log.log);
			log.AppendRecordHead(time);
			*log.log << '\t';
			Detail::DumpMultiLineString(message, *log.log);
			message.clear();
		}

	};

	////////////////////////////////////////////////////////////////////////////////

	template<typename RecordT>
	class AsyncLog : private boost::noncopyable {

	protected:

		typedef RecordT Record;

	private:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;
		typedef boost::condition_variable Condition;

		struct Buffer {
			
			std::vector<Record> records;
			typename std::vector<Record>::iterator end;
		
			Buffer()
					: end(records.begin()) {
				//...//
			}
		
		};

	public:

		AsyncLog()
				: m_currentBuffer(&m_buffers.first) {
			//...//
		}

		~AsyncLog() {
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
				if (!m_writeThread) {
					return;
				}
				auto thread = m_writeThread;
				m_writeThread.reset();
				m_condition.notify_all();
				lock.unlock();
				thread->join();
			} catch (...) {
				AssertFailNoException();
			}
		}

	public:

		template<typename T>
		void AppendRecord(
					const boost::posix_time::ptime &time,
					const T &param) {
			const Lock lock(m_mutex);
			if (m_currentBuffer->end == m_currentBuffer->records.end()) {
				m_currentBuffer->records.emplace(
					m_currentBuffer->records.end());
				m_currentBuffer->end = m_currentBuffer->records.end();
				m_currentBuffer->records.back().Save(time, param);
			} else {
				m_currentBuffer->end->Save(time, param);
				++m_currentBuffer->end;
			}
			m_condition.notify_one();
		}

	private:

		void WriteTask() {

			Lock lock(m_mutex);

			m_condition.notify_all();
	
			for ( ; ; ) {

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
						record->Flush(m_log);
					}
					currentBuffer->end = currentBuffer->records.begin();
				}
				lock.lock();

				if (m_currentBuffer->records.begin() != m_currentBuffer->end) {
					continue;
				} else if (!m_writeThread) {
					break;
				}
				
				m_condition.wait(lock);
		
			}
		
		}


	private:

		std::pair<Buffer, Buffer> m_buffers;
		Buffer *m_currentBuffer;

		trdk::Lib::LogState m_log;

		Mutex m_mutex;
		Condition m_condition;
		boost::shared_ptr<boost::thread> m_writeThread;

	};


} }
