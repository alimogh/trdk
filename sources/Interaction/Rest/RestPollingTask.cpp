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
#include "RestPollingTask.hpp"
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
  m_log.Debug("Starting pulling task...");
  try {
    Lock lock(m_mutex);
    while (!m_isStopped) {
      const auto &nextStartTime = ch::system_clock::now() + m_pollingInterval;
      m_task();
      m_condition.wait_until(lock, nextStartTime);
    }
  } catch (...) {
    AssertFailNoException();
    throw;
  }
  m_log.Debug("Pulling task completed.");
}
