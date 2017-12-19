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
#include "PullingTask.hpp"
#include "PullingSettings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace pt = boost::posix_time;
namespace ch = boost::chrono;

PullingTask::PullingTask(const PullingSetttings &setttings,
                         ModuleEventsLog &log)
    : m_log(log),
      m_pullingInterval(
          ch::microseconds(setttings.GetInterval().total_microseconds())),
      m_isAccelerated(false) {}

PullingTask::~PullingTask() {
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

void PullingTask::AddTask(const std::string &&name,
                          size_t priority,
                          const boost::function<bool()> &&task,
                          size_t frequency) {
  ScheduleTaskSetting(std::move(name), priority, std::move(task), frequency,
                      false);
}

void PullingTask::ReplaceTask(const std::string &&name,
                              size_t priority,
                              const boost::function<bool()> &&task,
                              size_t frequency) {
  ScheduleTaskSetting(std::move(name), priority, std::move(task), frequency,
                      true);
}

void PullingTask::ScheduleTaskSetting(const std::string &&name,
                                      size_t priority,
                                      const boost::function<bool()> &&task,
                                      size_t frequency,
                                      bool replace) {
  const Lock lock(m_mutex);
  m_newTasks.emplace_back(
      Task{std::move(name), priority, std::move(task), frequency}, replace);
  if (!m_thread) {
    m_thread = boost::thread(boost::bind(&PullingTask::RunTasks, this));
  }
}

void PullingTask::SetTasks() {
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
      *it = std::move(newTask.first);
    } else {
      m_tasks.emplace_back(std::move(newTask.first));
    }

    std::sort(m_tasks.begin(), m_tasks.end(), [](const Task &a, const Task &b) {
      return a.priority < b.priority;
    });
  }

  m_newTasks.clear();
  m_newTasks.shrink_to_fit();
  m_tasks.shrink_to_fit();
}

void PullingTask::AccelerateNextPulling() {
  m_isAccelerated = true;
  m_condition.notify_all();
}

void PullingTask::RunTasks() {
  m_log.Debug("Starting pulling task...");

  try {
    Lock lock(m_mutex);

    SetTasks();
    lock.unlock();

    while (!m_tasks.empty()) {
      const auto nextStartTime = ch::system_clock::now() + m_pullingInterval;

      bool isAccelerated = true;
      isAccelerated =
          m_isAccelerated.compare_exchange_weak(isAccelerated, false);
      if (isAccelerated) {
        lock.lock();
        SetTasks();
        lock.unlock();
      }

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
      lock.unlock();
    }
  } catch (const std::exception &ex) {
    m_log.Error("Fatal error in the pulling task: \"%1%\".", ex.what());
    throw;
  } catch (...) {
    m_log.Error("Fatal unknown error in the pulling task.");
    AssertFailNoException();
    throw;
  }
  m_log.Debug("Pulling task is completed.");
}

bool PullingTask::RunTask(Task &task, bool isAccelerated) const {
  if (!isAccelerated && task.skipCount > 0) {
    --task.skipCount;
    return false;
  }

  task.skipCount = task.frequency;

  bool isCompleted;
  try {
    isCompleted = !task.task();
  } catch (const std::exception &) {
    if (++task.numberOfErrors <= 2) {
      try {
        throw;
      } catch (const Interactor::CommunicationError &ex) {
        m_log.Warn(
            "%1% task \"%2%\" error: \"%3%\".",
            task.numberOfErrors == 1 ? "Pulling" : "Repeated pulling",  // 1
            task.name,                                                  // 2
            ex.what());                                                 // 3
      } catch (const std::exception &ex) {
        m_log.Error(
            "%1% task \"%2%\" error: \"%3%\".",
            task.numberOfErrors == 1 ? "Pulling" : "Repeated pulling",  // 1
            task.name,                                                  // 2
            ex.what());                                                 // 3
      }
    }
    return false;
  } catch (...) {
    AssertFailNoException();
    throw;
  }

  if (task.numberOfErrors > 1) {
    m_log.Info("Pulling task \"%1%\" restored.", task.name);
  }
  task.numberOfErrors = 0;

  return isCompleted;
}
