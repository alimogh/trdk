/**************************************************************************
 *   Created: 2014/11/09 14:31:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "AsyncLog.hpp"
#include "Security.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;

////////////////////////////////////////////////////////////////////////////////

class AsyncLog::WriteTask : private boost::noncopyable {

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
					m_log.Write(
						record.GetTag(),
						record.GetTime(),
						record.GetThreadId(),
						nullptr,
						record.GetMessage());
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

AsyncLog::AsyncLog()
		: m_writeTask(nullptr) {
	m_queue.activeBuffer = &m_queue.buffers.first;
}

AsyncLog::~AsyncLog() {
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

std::string AsyncLog::Record::GetMessage() const {
	
	boost::format format(m_message);
	
	foreach (const auto &param, m_params) {
	
		const ParamType &type = boost::get<0>(param);
		const boost::any &val = boost::get<1>(param);
	
		static_assert(numberOfCurrencies == 5, "Parameter type list changed.");
		switch (type) {

			case PT_INT8:
				format % boost::any_cast<int8_t>(val);
				break;
			case PT_UINT8:
				format % boost::any_cast<uint8_t>(val);
				break;

			case PT_INT16:
				format % boost::any_cast<int16_t>(val);
				break;
			case PT_UINT16:
				format % boost::any_cast<uint16_t>(val);
				break;

			case PT_INT32:
				format % boost::any_cast<int32_t>(val);
				break;
			case PT_UINT32:
				format % boost::any_cast<uint32_t>(val);
				break;

			case PT_INT64:
				format % boost::any_cast<int64_t>(val);
				break;
			case PT_UINT64:
				format % boost::any_cast<uint64_t>(val);
				break;

			case PT_FLOAT:
				format % boost::any_cast<float>(val);
				break;
			case PT_DOUBLE:
				format % boost::any_cast<double>(val);
				break;

			case PT_STRING:
				format % boost::any_cast<const std::string &>(val);
				break;
			case PT_PCHAR:
				format % boost::any_cast<const char *>(val);
				break;

			case PT_PTIME:
				format % boost::any_cast<const pt::ptime &>(val);
				break;

			case PT_SECURITY:
				format % *boost::any_cast<const Security *>(val);
				break;

			default:
				AssertEq(PT_UINT64, type);
		}

	}

	return std::move(format.str());

}

void AsyncLog::EnableStream(std::ostream &os) {
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

void AsyncLog::WaitForFlush() const throw() {
	try {
		m_writeTask->WaitForFlush();
	} catch (...) {
		AssertFailNoException();
	}
}

////////////////////////////////////////////////////////////////////////////////

ModuleAsyncLog::ModuleAsyncLog(const std::string &name, trdk::AsyncLog &log)
	:	m_name(name),
		m_namePch(m_name.c_str()),
		m_log(log) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////
