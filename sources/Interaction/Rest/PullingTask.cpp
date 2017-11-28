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
  AssertLt(0, m_tasks.size());
  try {
    {
      const Lock lock(m_mutex);
      AssertLt(0, m_tasks.size());
      m_tasks.clear();
    }
    m_condition.notify_all();
    m_thread->join();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

bool PullingTask::AddTask(const std::string &name,
                          size_t priority,
                          const boost::function<bool()> &task,
                          size_t frequency) {
  return SetTask(name, priority, task, frequency, false);
}
void PullingTask::ReplaceTask(const std::string &name,
                              size_t priority,
                              const boost::function<bool()> &task,
                              size_t frequency) {
  SetTask(name, priority, task, frequency, true);
}
bool PullingTask::SetTask(const std::string &name,
                          size_t priority,
                          const boost::function<bool()> &task,
                          size_t frequency,
                          bool replace) {
  {
    const Lock lock(m_mutex);
    AssertNe(m_tasks.empty(), m_thread ? true : false);

    const auto &it =
        std::find_if(m_tasks.begin(), m_tasks.end(),
                     [&name](const Task &task) { return task.name == name; });

    if (it != m_tasks.cend()) {
      if (!replace) {
        AssertEq(it->priority, priority);
        return false;
      }
      *it = Task{name, priority, task, frequency};
    } else {
      m_tasks.emplace_back(Task{name, priority, task, frequency});
    }

    std::sort(m_tasks.begin(), m_tasks.end(), [](const Task &a, const Task &b) {
      return a.priority < b.priority;
    });

    if (!m_thread) {
      m_thread = boost::thread(boost::bind(&PullingTask::Run, this));
    }
  }

  AccelerateNextPulling();

  return true;
}

void PullingTask::AccelerateNextPulling() {
  m_isAccelerated = true;
  m_condition.notify_all();
}

void PullingTask::Run() {
  m_log.Debug("Starting pulling task...");

  try {
    Lock lock(m_mutex);

    while (!m_tasks.empty()) {
      const auto nextStartTime = ch::system_clock::now() + m_pullingInterval;

      bool isAccelerated = true;
      isAccelerated =
          m_isAccelerated.compare_exchange_weak(isAccelerated, false);

      for (auto it = m_tasks.begin(); it != m_tasks.cend();) {
        auto &task = *it++;

        if (!isAccelerated && task.skipCount > 0) {
          --task.skipCount;
          continue;
        }

        task.skipCount = task.frequency;

        bool isCompleted;
        try {
          isCompleted = !task.task();
        } catch (const std::exception &ex) {
          isCompleted = false;
          if (++task.numberOfErrors <= 2) {
            m_log.Error(
                "%1% task \"%2%\" error: \"%3%\".",
                task.numberOfErrors == 1 ? "Pulling" : "Repeated pulling",  // 1
                task.name,                                                  // 2
                ex.what());                                                 // 3
          }
          continue;
        }

        if (task.numberOfErrors > 1) {
          m_log.Info("Pulling task \"%1%\" restored.", task.name);
        }
        task.numberOfErrors = 0;

        if (isCompleted) {
          const auto pos = std::distance(m_tasks.begin(), std::prev(it));
          m_tasks.erase(m_tasks.begin() + pos);
          it = m_tasks.begin() + pos;
        }
      }

      if (!m_isAccelerated) {
        m_condition.wait_until(lock, nextStartTime);
      }
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
