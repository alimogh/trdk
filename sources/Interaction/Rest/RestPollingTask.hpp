/*******************************************************************************
 *   Created: 2017/10/10 22:38:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace Rest {

class PollingTask : private boost::noncopyable {
 private:
  typedef boost::mutex Mutex;
  typedef Mutex::scoped_lock Lock;

 public:
  explicit PollingTask(const boost::function<void()> &task,
                       const boost::posix_time::time_duration &pollingInterval,
                       ModuleEventsLog &);
  ~PollingTask();

 private:
  void Run();

 private:
  ModuleEventsLog &m_log;
  const boost::function<void()> m_task;
  bool m_isStopped;
  Mutex m_mutex;
  boost::condition_variable m_condition;
  const boost::chrono::microseconds m_pollingInterval;
  boost::thread m_thread;
};
}
}
}