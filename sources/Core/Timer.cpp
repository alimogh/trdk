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
}

class Timer::Implementation : private boost::noncopyable {
 public:
  struct Task {
    Scope::Id scope;
    boost::function<void()> callback;
    pt::ptime time;
  };

  const Context &m_context;

  bool m_isStopped;

  Mutex m_mutex;
  boost::condition_variable m_condition;
  boost::thread m_thread;

  std::list<Task> m_tasks;
  pt::ptime m_nearestEvent;

  const pt::time_duration m_utcDiff;

  explicit Implementation(const Context &context)
      : m_context(context),
        m_isStopped(true),
        m_utcDiff(m_context.GetSettings().GetTimeZone()->base_utc_offset()) {}

  ~Implementation() {
    if (!m_isStopped) {
      try {
        {
          const Lock lock(m_mutex);
          Assert(!m_isStopped);
          m_isStopped = false;
        }
        m_condition.notify_all();
        m_thread.join();
      } catch (...) {
        AssertFailNoException();
        terminate();
      }
    }
  }

  void Schedule(const pt::time_duration &time,
                const boost::function<void()> &callback,
                Scope &scope) {
    {
      Lock lock(m_mutex);
      m_tasks.emplace_back(
          Task{scope.m_id, callback, m_context.GetCurrentTime() + time});
      m_context.GetTradingLog().Write("timer", "scheduling\t%1%\t%2%",
                                      [this](TradingRecord &record) {
                                        record % m_tasks.back().time  // 1
                                            % m_tasks.back().scope;   // 2
                                      });
      if (m_nearestEvent == pt::not_a_date_time ||
          m_tasks.back().time < m_nearestEvent) {
        m_nearestEvent = m_tasks.back().time;
      }
      if (m_isStopped) {
        m_thread.join();
        m_thread = boost::thread(
            boost::bind(&Implementation::ExecuteScheduling, this));
        m_isStopped = false;
      }
    }
    m_condition.notify_all();
  }

  void ExecuteScheduling() {
    m_context.GetLog().Debug("Started timer task.");

    try {
      Lock lock(m_mutex);
      while (!m_isStopped) {
        if (m_nearestEvent != pt::not_a_date_time &&
            m_nearestEvent <= m_context.GetCurrentTime()) {
          m_nearestEvent = pt::not_a_date_time;

          for (auto it = m_tasks.begin(); it != m_tasks.cend();) {
            Assert(it->time != pt::not_a_date_time);

            if (m_context.GetCurrentTime() < it->time) {
              if (m_nearestEvent == pt::not_a_date_time ||
                  it->time < m_nearestEvent) {
                m_nearestEvent = it->time;
              }
              ++it;
              continue;
            }

            m_context.GetTradingLog().Write("timer", "exec\t%1%\t%2%",
                                            [&it](TradingRecord &record) {
                                              record % it->time  // 1
                                                  % it->scope;   // 2
                                            });

            it->callback();
            m_tasks.erase(std::prev(++it));
          }
          AssertEq(m_nearestEvent == pt::not_a_date_time, m_tasks.empty());
        }

        m_nearestEvent != pt::not_a_date_time
            ? m_condition.timed_wait(lock, m_nearestEvent - m_utcDiff)
            : m_condition.wait(lock);
      }
      if (!m_tasks.empty()) {
        m_context.GetLog().Debug(
            "Timer has %1% uncompleted tasks, nearest at %2%.", m_tasks.size(),
            m_nearestEvent);
      }
    } catch (...) {
      AssertFailNoException();
      throw;
    }
    m_context.GetLog().Debug("Timer task is completed.");
  }
};

namespace {
boost::atomic_uintmax_t lastScopeId(0);
}

Timer::Scope::Scope() : m_id(lastScopeId++), m_timer(nullptr) {}

Timer::Scope::Scope(const Id &id) : m_id(id), m_timer(nullptr) {}

Timer::Scope::~Scope() { Cancel(); }

size_t Timer::Scope::Cancel() noexcept {
  if (!m_timer) {
    return 0;
  }
  size_t result = 0;
  try {
    auto &list = m_timer->m_pimpl->m_tasks;
    const Lock lock(m_timer->m_pimpl->m_mutex);
    for (auto it = list.begin(); it != list.cend();) {
      if (it->scope != m_id) {
        ++it;
        continue;
      }
      ++result;
      m_timer->m_pimpl->m_context.GetTradingLog().Write(
          "timer", "cancel\t%1%\t%2%", [&it](TradingRecord &record) {
            record % it->time  // 1
                % it->scope;   // 2
          });
      list.erase(std::prev(++it));
    }
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
  return result;
}

Timer::Timer(const Context &context)
    : m_pimpl(boost::make_unique<Implementation>(context)) {}

Timer::~Timer() = default;

void Timer::Schedule(const pt::time_duration &time,
                     const boost::function<void()> &callback,
                     Scope &scope) {
  Assert(!scope.m_timer || scope.m_timer == this);
  Assert(time != pt::not_a_date_time);
  if (scope.m_timer && scope.m_timer != this) {
    throw LogicError(
        "Failed to use timer scope that already attached to another timer "
        "object");
  }
  m_pimpl->Schedule(time, callback, scope);
  scope.m_timer = this;
}
