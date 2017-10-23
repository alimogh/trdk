/*******************************************************************************
 *   Created: 2017/10/10 22:56:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PollingTask.hpp"
#include "Core/EventsLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace pt = boost::posix_time;
namespace ch = boost::chrono;

PollingTask::PollingTask(const boost::function<void()> &task,
                         const pt::time_duration &pollingInterval,
                         ModuleEventsLog &log)
    : m_log(log),
      m_task(task),
      m_isStopped(false),
      m_pollingInterval(ch::microseconds(pollingInterval.total_microseconds())),
      m_thread(boost::bind(&PollingTask::Run, this)) {}

PollingTask::~PollingTask() {
  try {
    {
      const Lock lock(m_mutex);
      Assert(!m_isStopped);
      m_isStopped = true;
    }
    m_condition.notify_all();
    m_thread.join();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void PollingTask::Run() {
  m_log.Debug("Starting polling task...");
  try {
    size_t numberOfErrors = 0;
    Lock lock(m_mutex);
    while (!m_isStopped) {
      auto nextStartTime = ch::system_clock::now() + m_pollingInterval;
      try {
        m_task();
        numberOfErrors = 0;
      } catch (const trdk::Lib::Exception &ex) {
        ++numberOfErrors;
        m_log.Error("Polling task error: \"%1%\" (%2%).", ex.what(),
                    numberOfErrors);
        nextStartTime += m_pollingInterval *
                         std::min(static_cast<size_t>(30), numberOfErrors);
      }
      m_condition.wait_until(lock, nextStartTime);
    }
  } catch (const std::exception &ex) {
    m_log.Error("Fatal error in the polling task: \"%1%\".", ex.what());
    throw;
  } catch (...) {
    m_log.Error("Fatal unknown error in the polling task.");
    AssertFailNoException();
    throw;
  }
  m_log.Debug("Polling task completed.");
}
