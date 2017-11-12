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

class PullingTask : private boost::noncopyable {
 private:
  typedef boost::mutex Mutex;
  typedef Mutex::scoped_lock Lock;

  struct Task {
    std::string name;
    size_t priority;
    boost::function<bool()> task;
    size_t frequency;
    size_t numberOfErrors;
    size_t skipCount;
  };

 public:
  explicit PullingTask(const boost::posix_time::time_duration &pullingInterval,
                       ModuleEventsLog &);
  ~PullingTask();

 public:
  bool AddTask(const std::string &name,
               size_t priority,
               const boost::function<bool()> &task,
               size_t frequency);
  void ReplaceTask(const std::string &name,
                   size_t priority,
                   const boost::function<bool()> &task,
                   size_t frequency);
  void AccelerateNextPulling();

 private:
  void Run();

  bool SetTask(const std::string &name,
               size_t priority,
               const boost::function<bool()> &task,
               size_t frequency,
               bool replace);

 private:
  ModuleEventsLog &m_log;
  Mutex m_mutex;
  boost::condition_variable m_condition;
  const boost::chrono::microseconds m_pullingInterval;
  std::vector<Task> m_tasks;
  boost::optional<boost::thread> m_thread;
  boost::atomic_bool m_isAccelerated;
};
}
}
}