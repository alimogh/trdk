/*******************************************************************************
 *   Created: 2017/11/12 13:43:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FloodControl.hpp"

using namespace trdk;
using namespace trdk::Interaction::Rest;

namespace pt = boost::posix_time;
namespace r = trdk::Interaction::Rest;

////////////////////////////////////////////////////////////////////////////////

namespace {
void ReportDelay(const pt::ptime &startTime,
                 const pt::ptime &actionTime,
                 const pt::time_duration &criticalLevelStep,
                 bool isPriority,
                 size_t queueSize,
                 ModuleEventsLog &log) {
  AssertLe(startTime, actionTime);
  const auto delay = actionTime - startTime;
  if (delay < criticalLevelStep) {
    return;
  }

  const char *const message =
      "Rate limit exceeded (controlled delay: %1%, queue size: %2%).";

  isPriority ? delay > criticalLevelStep * 4
                   ? log.Error(message, delay, queueSize)
                   : delay > criticalLevelStep * 2
                         ? log.Warn(message, delay, queueSize)
                         : log.Info(message, delay, queueSize)
             : delay > criticalLevelStep * 6
                   ? log.Error(message, delay, queueSize)
                   : delay > criticalLevelStep * 4
                         ? log.Warn(message, delay, queueSize)
                         : delay > criticalLevelStep * 2
                               ? log.Info(message, delay, queueSize)
                               : log.Debug(message, delay, queueSize);
}
}  // namespace

////////////////////////////////////////////////////////////////////////////////

void FloodControl::OnRateLimitExceeded() {}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<FloodControl> r::CreateDisabledFloodControl() {
  class DisabledFloodControl : public FloodControl {
   public:
    virtual ~DisabledFloodControl() override = default;

   public:
    virtual void Check(bool, ModuleEventsLog &) override {}
  };

  return boost::make_unique<DisabledFloodControl>();
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<FloodControl> r::CreateFloodControlWithMinTimeBetweenRequests(
    const pt::time_duration &minTimeBetweenRequests,
    const pt::time_duration &criticalWaitTimeToReport) {
  class MinTimeBetweenRequestsFloodControl : public FloodControl {
   private:
    typedef boost::mutex Mutex;
    typedef Mutex::scoped_lock Lock;

   public:
    explicit MinTimeBetweenRequestsFloodControl(
        const pt::time_duration &minTimeBetweenRequests,
        const pt::time_duration &criticalWaitTimeToReport)
        : m_minTimeBetweenRequests(minTimeBetweenRequests),
          m_criticalWaitTimeToReport(criticalWaitTimeToReport),
          m_nextRequestTime(GetCurrentTime() + m_minTimeBetweenRequests) {}

    virtual ~MinTimeBetweenRequestsFloodControl() override = default;

   public:
    virtual void Check(bool isPriority, ModuleEventsLog &log) override {
      Lock lock(m_mutex);

      const auto startTime = GetCurrentTime();

      if (isPriority) {
        m_nextRequestTime = startTime + m_minTimeBetweenRequests;
        return;
      }

      auto now = startTime;
      auto timeToWait = m_nextRequestTime - now;

      while (!timeToWait.is_negative()) {
        lock.unlock();
        boost::this_thread::sleep(timeToWait);
        lock.lock();
        now = GetCurrentTime();
        timeToWait = m_nextRequestTime - now;
      }

      m_nextRequestTime = now + m_minTimeBetweenRequests;

      ReportDelay(startTime, now, m_criticalWaitTimeToReport, false, 0, log);
    }

   private:
    static pt::ptime GetCurrentTime() {
      return pt::microsec_clock::universal_time();
    }

   private:
    const boost::posix_time::time_duration m_minTimeBetweenRequests;
    const boost::posix_time::time_duration m_criticalWaitTimeToReport;
    Mutex m_mutex;
    boost::posix_time::ptime m_nextRequestTime;
  };

  return boost::make_unique<MinTimeBetweenRequestsFloodControl>(
      minTimeBetweenRequests, criticalWaitTimeToReport);
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<FloodControl> r::CreateFloodControlWithMaxRequestsPerPeriod(
    size_t maxNumberOfRequest,
    const pt::time_duration &period,
    const pt::time_duration &criticalWaitTimeToReport) {
  class MaxRequestsPerPeriodFloodControl : public FloodControl {
   private:
    typedef boost::mutex Mutex;
    typedef Mutex::scoped_lock Lock;

   public:
    MaxRequestsPerPeriodFloodControl(
        size_t maxNumberOfRequest,
        const pt::time_duration &period,
        const pt::time_duration &criticalWaitTimeToReport)
        : m_period(period),
          m_events(maxNumberOfRequest),
          m_criticalWaitTimeToReport(criticalWaitTimeToReport) {}

    virtual ~MaxRequestsPerPeriodFloodControl() override = default;

   public:
    virtual void Check(bool isPriority, ModuleEventsLog &log) override {
      Lock lock(m_mutex);

      const auto &startTime = GetCurrentTime();
      if (isPriority) {
        m_events.push_back(startTime);
        return;
      }

      auto now = startTime;
      FlushEvents(now - m_period);

      while (m_events.full()) {
        AssertEq(m_events.capacity(), m_events.size());
        const auto timeToWait = m_events.front() + m_period - now;
        AssertLt(pt::microseconds(0), timeToWait);
        Assert(!timeToWait.is_negative());
        lock.unlock();
        boost::this_thread::sleep(timeToWait);
        lock.lock();
        now = GetCurrentTime();
        FlushEvents(now - m_period);
      }

      m_events.push_back(now);

      ReportDelay(startTime, now, m_criticalWaitTimeToReport, false,
                  m_events.size(), log);
    }

    virtual void OnRateLimitExceeded() override {
      const Lock lock(m_mutex);
      const auto &now = GetCurrentTime();
      FlushEvents(now - m_period);
      const auto lastTime =
          (!m_events.empty() ? m_events.front() : now) + m_period;
      do {
        m_events.push_back(lastTime);
      } while (!m_events.full());
    }

   private:
    static pt::ptime GetCurrentTime() {
      return pt::microsec_clock::universal_time();
    }

    void FlushEvents(const pt::ptime &startTime) {
      while (!m_events.empty() && m_events.front() <= startTime) {
        m_events.pop_front();
      }
    }

   private:
    Mutex m_mutex;
    const pt::time_duration m_period;
    const pt::time_duration m_criticalWaitTimeToReport;
    boost::circular_buffer<pt::ptime> m_events;
  };

  return boost::make_unique<MaxRequestsPerPeriodFloodControl>(
      maxNumberOfRequest, period, criticalWaitTimeToReport);
}

////////////////////////////////////////////////////////////////////////////////
