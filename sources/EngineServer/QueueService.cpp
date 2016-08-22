/*******************************************************************************
 *   Created: 2015/12/11 02:21:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "QueueService.hpp"
#include "Core/EventsLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::EngineServer;

//////////////////////////////////////////////////////////////////////////

QueueService::Item::Item(size_t index, const Callback &&callback)
	: m_index(index)
	, m_attemptNo(0)
#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		, m_isDequeued(false)
#	endif
	, m_callback(callback) {
	//...//
}

bool QueueService::Item::Dequeue() {
	Assert(!m_isDequeued);
	const auto &result = m_callback(m_index, ++m_attemptNo, false);
#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		m_isDequeued = result;
#	endif
	return result;
}

void QueueService::Item::Dump() {
	Assert(!m_isDequeued);
	Verify(m_callback(m_index, ++m_attemptNo, true));
#	ifdef BOOST_ENABLE_ASSERT_HANDLER
		m_isDequeued = true;
#	endif
}

//////////////////////////////////////////////////////////////////////////

QueueService::Queue::Queue()
	: logicalSize(0) {
	//...//
}

void QueueService::Queue::Reset() {
	data.clear();
	logicalSize = 0;
}

QueueService::Queue::Data::iterator QueueService::Queue::GetLogicalBegin() {
	AssertLe(logicalSize, data.size());
	const auto &end = data.end();
	const auto &begin = end - logicalSize;
	Assert(data.begin() + data.size() - logicalSize == begin);
	return begin;
}

//////////////////////////////////////////////////////////////////////////

QueueService::QueueService(EventsLog &log)
	: m_log(log)
	, m_flushFlag(false)
	, m_nextRecordNumber(1)
	, m_currentQueue(&m_queues.first)
	, m_isStopped(true) {
	//...//
}

QueueService::~QueueService() {
	// All records should be flushed or dumped by engine:
	AssertEq(0, GetSize());
	try {
		Stop(true);
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
}

size_t QueueService::GetSize() const {
	return m_queues.first.logicalSize + m_queues.second.logicalSize;
}

bool QueueService::IsEmpty() const {
	return GetSize() == 0;
}

bool QueueService::IsStarted() const {
	const StateLock lock(m_stateMutex);
	return !m_isStopped;
}

void QueueService::Start() {
	const StateLock lock(m_stateMutex);
	if (!m_isStopped) {
		throw Exception("Alerady is started");
	}
	m_thread = boost::thread(
		[this]() {
			m_log.Debug("Started Drop Copy queue task.");
			m_isStopped = false;
			try {
				RunDequeue();
			} catch (...) {
				AssertFailNoException();
				m_log.Error("Fatal error in the Drop Copy queue task.");
				throw;
			}
			m_log.Debug("Drop Copy queue task completed.");
		});
}

void QueueService::Stop(bool sync) {
	{
		const StateLock lock(m_stateMutex);
		if (m_isStopped && !sync) {
			throw Exception("Alerady is stopped");
		}
		m_isStopped = true;
	}
	m_dataCondition.notify_all();
	if (sync) {
		m_thread.join();
	}
}

void QueueService::Flush() {
	StateLock lock(m_stateMutex);
	if (m_flushFlag) {
		throw LogicError("Only one thread can flush Drop Copy");
	}
	if (IsEmpty()) {
		return;
	}
	m_flushFlag = true;
	while (m_flushFlag && !m_isStopped && !IsEmpty()) {
		m_log.Info("Flushing %1% Drop Copy messages...", GetSize());
		// Will wait as long as it will require, without timeout...
		m_dataCondition.wait(lock);
	}
}

void QueueService::Enqueue(Callback &&callback) {
	{
		const StateLock lock(m_stateMutex);
		m_currentQueue->data.emplace_back(
			m_nextRecordNumber,
			std::move(callback));
		++m_nextRecordNumber;
		++m_currentQueue->logicalSize;
	}
	m_dataCondition.notify_one();
}

void QueueService::RunDequeue() {

	StateLock stateLock(m_stateMutex);

	while (!m_isStopped) {

		size_t prevSize = 0;
		size_t numberOfRisingIterations = 0;
		while (!m_isStopped && !IsEmpty()) {

			Queue *listToRead;
			{
				const DequeueLock dequeueLock(m_dequeueMutex);
				listToRead = m_currentQueue == &m_queues.first
					?	!m_queues.second.data.empty()
						?	&m_queues.second
						:	&m_queues.first
					:	!m_queues.first.data.empty()
						?	&m_queues.first
						:	&m_queues.second;
				AssertLt(0, listToRead->data.size());
				AssertLt(0, listToRead->logicalSize);
				AssertGe(listToRead->data.size(), listToRead->logicalSize);
			}
			if (listToRead == m_currentQueue) {
				m_currentQueue = m_currentQueue == &m_queues.first
					?	&m_queues.second
					:	&m_queues.first;
			}

			stateLock.unlock();

			{

				const DequeueLock dequeueLock(m_dequeueMutex);

				if (prevSize) {
					if (prevSize < listToRead->logicalSize) {
						if (++numberOfRisingIterations >= 3) {
							m_log.Warn(
								"Drop Copy queue size is rising"
									": %1% -> %2% (%3%) at %4%.",
									prevSize,
									listToRead->logicalSize,
									GetSize(),
									numberOfRisingIterations);
						}
					} else {
						if (numberOfRisingIterations >= 3) {
							m_log.Debug(
								"Drop Copy queue size rising is stopped"
									": %1% -> %2% (%3%) at %4%.",
									prevSize,
									listToRead->logicalSize,
									GetSize(),
									numberOfRisingIterations);
						}
						numberOfRisingIterations = 0;
					}
				}
				prevSize = listToRead->logicalSize;
	
				Assert(!listToRead->data.empty());
				AssertGe(listToRead->data.size(), listToRead->logicalSize);

				for (
						auto it = listToRead->GetLogicalBegin();
						!m_isStopped && it != listToRead->data.cend();
						++it) {
					AssertLt(0, listToRead->logicalSize);
					if (!it->Dequeue()) {
						break;
					}
					--listToRead->logicalSize;
					AssertGt(listToRead->data.size(), listToRead->logicalSize);
				}
				if (!listToRead->logicalSize) {
					listToRead->Reset();
				}

			}

			stateLock.lock();

			if (listToRead->logicalSize) {
				break;
			}

		}

		if (m_flushFlag && IsEmpty()) {
			m_flushFlag = false;
			m_dataCondition.notify_all();
		}

		if (m_isStopped) {
			if (m_flushFlag) {
				m_flushFlag = false;
				m_dataCondition.notify_all();
			}
			break;
		}

		m_dataCondition.wait(stateLock);
		AssertEq(m_currentQueue->data.size(), m_currentQueue->logicalSize);
		AssertGe(GetSize(), m_currentQueue->data.size());

	}

}

void QueueService::OnConnectionRestored() {
	m_dataCondition.notify_one();
}

void QueueService::Dump() {

	const StateLock stateLock(m_stateMutex);
	const DequeueLock dequeueLock(m_dequeueMutex);

	if (IsEmpty()) {
		return;
	}

	m_log.Warn(
		"Drop Copy queue has %1% unsaved records. Dumping unsaved records...",
		GetSize());

	while (!IsEmpty()) {

		Queue *const listToRead =
			m_currentQueue == &m_queues.first
				?	!m_queues.second.data.empty()
					?	&m_queues.second
					:	&m_queues.first
				:	!m_queues.first.data.empty()
					?	&m_queues.first
					:	&m_queues.second;
		AssertLt(0, listToRead->data.size());
		AssertLt(0, listToRead->logicalSize);
		AssertGe(listToRead->data.size(), listToRead->logicalSize);
		if (listToRead == m_currentQueue) {
			m_currentQueue = m_currentQueue == &m_queues.first
				?	&m_queues.second
				:	&m_queues.first;
		}

		Assert(!listToRead->data.empty());
		AssertGe(listToRead->data.size(), listToRead->logicalSize);
		for (
				auto it = listToRead->GetLogicalBegin();
				it != listToRead->data.cend();
				++it) {
			AssertLt(0, listToRead->logicalSize);
			it->Dump();
			--listToRead->logicalSize;
			AssertGt(listToRead->data.size(), listToRead->logicalSize);
		}

		AssertEq(0, listToRead->logicalSize);
		listToRead->Reset();

	}

	m_log.Info("Drop Copy record dumped.");

}

size_t QueueService::TakeRecordNumber() {
	const StateLock lock(m_stateMutex);
	return m_nextRecordNumber++;
}

//////////////////////////////////////////////////////////////////////////
