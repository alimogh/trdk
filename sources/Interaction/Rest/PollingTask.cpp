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
#include "PollingSettings.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction;
using namespace Rest;
namespace pt = boost::posix_time;
namespace ch = boost::chrono;

PollingTask::PollingTask(const PollingSettings &setttings, ModuleEventsLog &log)
    : m_log(log),
      m_pollingInterval(
          ch::microseconds(setttings.GetInterval().total_microseconds())),
      m_isAccelerated(false) {}

PollingTask::~PollingTask() {
  if (!m_thread) {
    AssertEq(0, m_tasks.size());
    return;
  }
  try {
    Lock lock(m_mutex);
    m_newTasks.clear();
    auto thread = std::move(*m_thread);
    m_thread = boost::none;
    lock.unlock();
    m_condition.notify_all();
    thread.join();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void PollingTask::AddTask(std::string name,
                          const size_t priority,
                          boost::function<bool()> task,
                          const size_t frequency,
                          const bool isAccelerable) {
  ScheduleTaskSetting(std::move(name), priority, std::move(task), frequency,
                      isAccelerable, false);
}

void PollingTask::ReplaceTask(std::string name,
                              const size_t priority,
                              boost::function<bool()> task,
                              const size_t frequency,
                              const bool isAccelerable) {
  ScheduleTaskSetting(std::move(name), priority, std::move(task), frequency,
                      isAccelerable, true);
}

void PollingTask::ScheduleTaskSetting(std::string name,
                                      const size_t priority,
                                      boost::function<bool()> task,
                                      const size_t frequency,
                                      const bool isAccelerable,
                                      bool replace) {
  const Lock lock(m_mutex);
  m_newTasks.emplace_back(Task{std::move(name), priority, isAccelerable,
                               std::move(task), frequency},
                          replace);
  if (!m_thread) {
    m_thread = boost::thread(boost::bind(&PollingTask::RunTasks, this));
  }
}

void PollingTask::SetTasks() {
  if (m_newTasks.empty()) {
    return;
  }

  for (const auto &newTask : m_newTasks) {
    const auto &it = std::find_if(m_tasks.begin(), m_tasks.end(),
                                  [&newTask](const Task &task) {
                                    return newTask.first.name == task.name;
                                  });

    if (it != m_tasks.cend()) {
      if (!newTask.second) {
        AssertEq(it->priority, newTask.first.priority);
        continue;
      }
      *it = newTask.first;
    } else {
      m_tasks.emplace_back(newTask.first);
    }

    std::sort(m_tasks.begin(), m_tasks.end(), [](const Task &a, const Task &b) {
      return a.priority < b.priority;
    });
  }

  m_newTasks.clear();
  m_newTasks.shrink_to_fit();
  m_tasks.shrink_to_fit();
}

void PollingTask::AccelerateNextPolling() {
  {
    const Lock lock(m_mutex);
    m_isAccelerated = true;
  }
  m_condition.notify_all();
}

void PollingTask::RunTasks() {
  StructuredException::SetupForThisThread();

  m_log.Debug("Starting polling task...");

  try {
    Lock lock(m_mutex);
    SetTasks();

    auto nextStartTime = ch::system_clock::now() + m_pollingInterval;
    while (!m_tasks.empty()) {
      const auto isAccelerated = m_isAccelerated;
      m_isAccelerated = false;
      if (!isAccelerated) {
        nextStartTime = ch::system_clock::now() + m_pollingInterval;
      }
      lock.unlock();

      for (auto it = m_tasks.begin(); it != m_tasks.cend();) {
        if (RunTask(*it++, isAccelerated)) {
          const auto pos = std::distance(m_tasks.begin(), std::prev(it));
          m_tasks.erase(m_tasks.begin() + pos);
          it = m_tasks.begin() + pos;
        }
      }

      lock.lock();
      if (!m_thread) {
        break;
      }
      if (!m_isAccelerated) {
        m_condition.wait_until(lock, nextStartTime);
      }
      SetTasks();
    }
  } catch (const std::exception &ex) {
    m_log.Error("Fatal error in the polling task: \"%1%\".", ex.what());
    throw;
  } catch (...) {
    m_log.Error("Fatal unknown error in the polling task.");
    AssertFailNoException();
    throw;
  }
  m_log.Debug("Polling task is completed.");
}

bool PollingTask::RunTask(Task &task, bool isAccelerated) const {
  if (!isAccelerated) {
    if (task.skipCount > 0) {
      --task.skipCount;
      return false;
    }
  } else if (!task.isAccelerable) {
    return false;
  }

  task.skipCount = task.frequency;

  bool isCompleted;
  try {
    isCompleted = !task.task();
  } catch (const std::exception &ex) {
    ++task.numberOfErrors;
    if (task.numberOfErrors <= 2) {
      try {
        throw;
      } catch (const CommunicationError &reEx) {
        m_log.Debug(
            R"(%1% task "%2%" error: "%3%".)",
            task.numberOfErrors == 1 ? "Polling" : "Repeated polling",  // 1
            task.name,                                                  // 2
            reEx.what());                                               // 3
      } catch (const std::exception &reEx) {
        m_log.Error(
            R"(%1% task "%2%" error: "%3%".)",
            task.numberOfErrors == 1 ? "Polling" : "Repeated polling",  // 1
            task.name,                                                  // 2
            reEx.what());                                               // 3
      }
    } else {
      const auto thinning = task.numberOfErrors <= 50
                                ? 10
                                : task.numberOfErrors <= 500 ? 50 : 100;
      if (!(task.numberOfErrors % thinning)) {
        m_log.Error(
            "Polling task \"%1%\" still gets an error at each iteration. Last "
            "error: \"%2%\". Number of errors: %3%.",
            task.name,             // 1
            ex.what(),             // 2
            task.numberOfErrors);  // 3
      }
    }
    return false;
  } catch (...) {
    AssertFailNoException();
    throw;
  }

  if (task.numberOfErrors > 1) {
    if (task.numberOfErrors >= 10) {
      m_log.Info("Polling task \"%1%\" restored after %2% errors.",
                 task.name,             // 1
                 task.numberOfErrors);  // 2
    } else {
      m_log.Debug("Polling task \"%1%\" restored after %2% errors.",
                  task.name,             // 1
                  task.numberOfErrors);  // 2
    }
    task.numberOfErrors = 0;
  }

  return isCompleted;
}
