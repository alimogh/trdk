/*******************************************************************************
 *   Created: 2017/10/24 15:51:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Timer.hpp"
#include "Context.hpp"
#include "Settings.hpp"
#include "TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;

namespace {
typedef boost::mutex Mutex;
typedef Mutex::scoped_lock Lock;

struct Task {
  Timer::Scope::Id scope;
  boost::function<void()> callback;
};
struct TimedTask : public Task {
  pt::ptime time;
  explicit TimedTask(const Timer::Scope::Id &scope,
                     const boost::function<void()> &callback,
                     const pt::ptime &time)
      : Task({scope, callback}), time(time) {}
};

size_t Erase(std::list<TimedTask> &list,
             const Timer::Scope::Id &scope,
             const Context &context) {
  size_t result = 0;
  for (auto it = list.begin(); it != list.cend();) {
    if (it->scope != scope) {
      ++it;
      continue;
    }
    ++result;
    context.GetTradingLog().Write(
        "Timer", "{'timer': {'cancel': {'time': '%1%', 'scope': %2%}}}",
        [&it](TradingRecord &record) {
          record % it->time  // 1
              % it->scope;   // 2
        });
    list.erase(std::prev(++it));
  }
  return result;
}

size_t Erase(std::vector<Task> &list, const Timer::Scope::Id &scope) {
  size_t result = 0;
  for (size_t i = 0; i < list.size();) {
    if (list[i].scope != scope) {
      ++i;
      continue;
    }
    ++result;
    list.erase(list.begin() + i);
  }
  return result;
}
}

class Timer::Implementation : private boost::noncopyable {
 public:
  const Context &m_context;

  Mutex m_mutex;
  boost::condition_variable m_condition;
  boost::optional<boost::thread> m_thread;

  std::vector<Task> m_immediateTasks;
  std::vector<Task> m_newImmediateTasks;
  std::list<TimedTask> m_newTimedTasks;
  std::vector<Scope::Id> m_removedTimedTasks;
  std::list<TimedTask> m_timedTasks;
  pt::ptime m_nearestEvent;

  const pt::time_duration m_utcDiff;

  bool m_isStopped;

  explicit Implementation(const Context &context)
      : m_context(context),
        m_utcDiff(m_context.GetSettings().GetTimeZone()->base_utc_offset()),
        m_isStopped(false) {}

  ~Implementation() {
    try {
      Stop();
    } catch (...) {
      AssertFailNoException();
      terminate();
    }
  }

  void Stop() {
    boost::optional<boost::thread> thread;
    {
      const Lock lock(m_mutex);
      m_thread.swap(thread);
    }
    if (thread) {
      m_condition.notify_all();
      thread->join();
    }
  }

  void Schedule(const pt::time_duration &time,
                const boost::function<void()> &&callback,
                Scope &scope) {
    {
      const Lock lock(m_mutex);
      if (m_isStopped) {
        return;
      }
      if (time != pt::not_a_date_time) {
        m_newTimedTasks.emplace_back(scope.m_id, std::move(callback),
                                     m_context.GetCurrentTime() + time);
        m_context.GetTradingLog().Write(
            "Timer", "{'timer': {'scheduling': {'time': '%1%', 'scope': %2%}}}",
            [this](TradingRecord &record) {
              record % m_newTimedTasks.back().time  // 1
                  % m_newTimedTasks.back().scope;   // 2
            });
      } else {
        m_newImmediateTasks.emplace_back(Task{scope.m_id, std::move(callback)});
        m_context.GetTradingLog().Write(
            "Timer", "{'timer': {'scheduling': {'scope': %1%}}}",
            [this](TradingRecord &record) {
              record % m_newTimedTasks.back().scope;
            });
      }
      if (!m_thread) {
        m_thread = boost::thread(
            boost::bind(&Implementation::ExecuteScheduling, this));
        return;
      }
    }
    m_condition.notify_all();
  }

  void SetNewTasks() {
    for (const auto &removedId : m_removedTimedTasks) {
      Erase(m_timedTasks, removedId, m_context);
    }
    m_removedTimedTasks.clear();

    for (auto &&task : m_newTimedTasks) {
      m_timedTasks.emplace_back(std::move(task));
      if (m_nearestEvent == pt::not_a_date_time ||
          m_timedTasks.back().time < m_nearestEvent) {
        m_nearestEvent = m_timedTasks.back().time;
      }
    }
    m_newTimedTasks.clear();

    m_immediateTasks.clear();
    m_immediateTasks.swap(m_newImmediateTasks);
  }

  void ExecuteScheduling() {
    m_context.GetLog().Debug("Started timer task.");

    try {
      Lock tasksLock(m_mutex);
      while (m_thread) {
        SetNewTasks();

        do {
          tasksLock.unlock();

          for (const auto &task : m_immediateTasks) {
            m_context.GetTradingLog().Write(
                "Timer", "{'timer': {'exec': {'scope': %1%}}}",
                [&task](TradingRecord &record) { record % task.scope; });
            try {
              task.callback();
            } catch (const std::exception &ex) {
              m_context.GetLog().Error("Scheduled task error: \"%1%\".",
                                       ex.what());
            } catch (...) {
              AssertFailNoException();
            }
          }

          if (m_nearestEvent != pt::not_a_date_time &&
              m_nearestEvent <= m_context.GetCurrentTime()) {
            m_nearestEvent = pt::not_a_date_time;

            for (auto it = m_timedTasks.begin(); it != m_timedTasks.cend();) {
              Assert(it->time != pt::not_a_date_time);

              if (m_context.GetCurrentTime() < it->time) {
                if (m_nearestEvent == pt::not_a_date_time ||
                    it->time < m_nearestEvent) {
                  m_nearestEvent = it->time;
                }
                ++it;
                continue;
              }

              m_context.GetTradingLog().Write(
                  "Timer", "{'timer': {'exec': {'time': '%1%', 'scope': %2%}}}",
                  [&it](TradingRecord &record) {
                    record % it->time  // 1
                        % it->scope;   // 2
                  });

              try {
                it->callback();
              } catch (const std::exception &ex) {
                m_context.GetLog().Error("Scheduled task error: \"%1%\".",
                                         ex.what());
              } catch (...) {
                AssertFailNoException();
              }
              m_timedTasks.erase(std::prev(++it));
            }
            AssertEq(m_nearestEvent == pt::not_a_date_time,
                     m_timedTasks.empty());
          }

          tasksLock.lock();
          SetNewTasks();
        } while (!m_immediateTasks.empty());

        m_nearestEvent != pt::not_a_date_time
            ? m_condition.timed_wait(tasksLock, m_nearestEvent - m_utcDiff)
            : m_condition.wait(tasksLock);
      }

      if (!m_timedTasks.empty() || !m_newTimedTasks.empty() ||
          !m_immediateTasks.empty() || !m_newImmediateTasks.empty()) {
        m_context.GetLog().Debug(
            "Timer has %1% uncompleted tasks, nearest at %2%.",
            m_timedTasks.size() + m_newTimedTasks.empty() +
                m_immediateTasks.size() + m_newImmediateTasks.empty(),
            m_immediateTasks.empty()
                ? boost::lexical_cast<std::string>(m_nearestEvent).c_str()
                : "\"immediately\"");
        Assert(m_isStopped);
        m_immediateTasks.clear();
        m_newImmediateTasks.clear();
        m_newTimedTasks.clear();
        m_removedTimedTasks.clear();
        m_timedTasks.clear();
      }
    } catch (...) {
      AssertFailNoException();
      throw;
    }
    m_context.GetLog().Debug("Timer task is completed.");
  }
};

namespace {
boost::atomic_size_t lastScopeId(0);
}

Timer::Scope::Scope() : m_id(lastScopeId++), m_timer(nullptr) {}

Timer::Scope::~Scope() { Cancel(); }

size_t Timer::Scope::Cancel() noexcept {
  if (!m_timer) {
    return 0;
  }
  try {
    //! @todo Add ability to set scope mode "blocked cancel" (for tasks in
    //! execution too).
    const Lock tasksLock(m_timer->m_pimpl->m_mutex);
    m_timer->m_pimpl->m_removedTimedTasks.emplace_back(m_id);
    return Erase(m_timer->m_pimpl->m_newTimedTasks, m_id,
                 m_timer->m_pimpl->m_context) +
           Erase(m_timer->m_pimpl->m_newImmediateTasks, m_id);
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

Timer::Timer(const Context &context)
    : m_pimpl(boost::make_unique<Implementation>(context)) {}

Timer::~Timer() = default;

void Timer::Schedule(const pt::time_duration &time,
                     const boost::function<void()> &&callback,
                     Timer::Scope &scope) const {
  Assert(!scope.m_timer || scope.m_timer == this);
  Assert(time != pt::not_a_date_time);
  if (scope.m_timer && scope.m_timer != this) {
    throw LogicError(
        "Failed to use timer scope that already attached to another timer "
        "object");
  }
  m_pimpl->Schedule(time, std::move(callback), scope);
  scope.m_timer = this;
}

void Timer::Schedule(const boost::function<void()> &&callback,
                     Scope &scope) const {
  m_pimpl->Schedule(pt::not_a_date_time, std::move(callback), scope);
}

void Timer::Stop() {
  {
    const Lock lock(m_pimpl->m_mutex);
    if (m_pimpl->m_isStopped) {
      throw LogicError("Timer already stopped");
    }
    m_pimpl->m_isStopped = true;
  }
  m_pimpl->Stop();
}
