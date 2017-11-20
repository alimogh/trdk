/**************************************************************************
 *   Created: 2012/10/24 13:23:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <boost/array.hpp>
#include <boost/chrono.hpp>

namespace trdk {
namespace Lib {
namespace TimeMeasurement {

////////////////////////////////////////////////////////////////////////////////

class Timer;
class Stat;

typedef size_t MilestoneIndex;
typedef intmax_t PeriodFromStart;

////////////////////////////////////////////////////////////////////////////////

enum StrategyMilestone {
  SM_DISPATCHING_DATA_STORE,
  SM_DISPATCHING_DATA_ENQUEUE,
  SM_DISPATCHING_DATA_DEQUEUE,
  SM_DISPATCHING_DATA_RAISE,
  SM_STRATEGY_WITHOUT_DECISION_1,
  SM_STRATEGY_WITHOUT_DECISION_2,
  SM_STRATEGY_DECISION_START_1,
  SM_STRATEGY_DECISION_START_2,
  SM_PRE_RISK_CONTROL_START,
  SM_PRE_RISK_CONTROL_COMPLETE,
  SM_STRATEGY_EXECUTION_START_1,
  SM_STRATEGY_EXECUTION_START_2,
  SM_STRATEGY_EXECUTION_COMPLETE_1,
  SM_STRATEGY_EXECUTION_COMPLETE_2,
  SM_POST_RISK_CONTROL_START,
  SM_POST_RISK_CONTROL_COMPLETE,
  SM_STRATEGY_EXECUTION_REPLY_1,
  SM_STRATEGY_EXECUTION_REPLY_2,
  numberOfStrategyMilestones
};
const std::string &GetMilestoneName(const StrategyMilestone &);

enum TradingSystemMilestone {
  TSM_ORDER_ENQUEUE,
  TSM_ORDER_PACK,
  TSM_ORDER_SEND,
  TSM_ORDER_SENT,
  TSM_ORDER_REPLY_RECEIVED,
  TSM_ORDER_REPLY_PROCESSED,
  numberOfTradingSystemMilestones
};
const std::string &GetMilestoneName(const TradingSystemMilestone &);

enum DispatchingMilestone {
  DM_COMPLETE_LIST,
  DM_COMPLETE_ALL,
  DM_NEW_DATA,
  numberOfDispatchingMilestones
};
const std::string &GetMilestoneName(const DispatchingMilestone &);

////////////////////////////////////////////////////////////////////////////////

class StatAccum {
 public:
  virtual ~StatAccum() {}

 public:
  virtual void AddMeasurement(const MilestoneIndex &,
                              const PeriodFromStart &) = 0;
};

////////////////////////////////////////////////////////////////////////////////

class Milestones {
 public:
  typedef boost::chrono::high_resolution_clock Clock;
  typedef Clock::time_point TimePoint;

 public:
  Milestones() {}

  explicit Milestones(const boost::shared_ptr<StatAccum> &stat)
      : m_start(Clock::now()), m_stat(stat) {}

  Milestones(const Milestones &rhs)
      : m_start(rhs.m_start), m_stat(rhs.m_stat) {}

  Milestones &operator=(const Milestones &rhs) {
    m_start = rhs.m_start;
    m_stat = rhs.m_stat;
    return *this;
  }

  operator bool() const { return m_stat ? true : false; }

 public:
  PeriodFromStart Measure(const MilestoneIndex &milestone) const throw() {
    if (!m_stat) {
      return 0;
    }
    try {
      return Add(milestone, GetNow());
    } catch (...) {
      AssertFailNoException();
      return 0;
    }
  }

  PeriodFromStart Add(const MilestoneIndex &milestone,
                      const TimePoint &now) const {
    if (!m_stat) {
      return 0;
    }
    const auto &period = CalcPeriod(m_start, now);
    m_stat->AddMeasurement(milestone, period);
    return period;
  }

 public:
  static TimePoint GetNow() { return Clock::now(); }

  static PeriodFromStart CalcPeriod(const TimePoint &start,
                                    const TimePoint &end) {
    typedef boost::chrono::microseconds Result;
    const auto period = end - start;
    return boost::chrono::duration_cast<Result>(period).count();
  }

 private:
  TimePoint m_start;
  boost::shared_ptr<StatAccum> m_stat;
};

////////////////////////////////////////////////////////////////////////////////

class MilestoneStat : private boost::noncopyable {
 public:
  MilestoneStat() : m_min(0), m_max(0), m_sum(0), m_size(0) {}

 public:
  operator bool() const { return m_size > 0; }

  MilestoneStat &operator|=(const PeriodFromStart &measurement) {
    for (;;) {
      auto prev = m_min.load(boost::memory_order_relaxed);
      if ((prev && measurement >= prev) ||
          m_min.compare_exchange_weak(prev, measurement,
                                      boost::memory_order_release,
                                      boost::memory_order_relaxed)) {
        break;
      }
    }
    for (;;) {
      auto prev = m_max.load(boost::memory_order_relaxed);
      if ((prev && measurement <= prev) ||
          m_max.compare_exchange_weak(prev, measurement,
                                      boost::memory_order_release,
                                      boost::memory_order_relaxed)) {
        break;
      }
    }
    for (;;) {
      auto prev = m_sum.load(boost::memory_order_relaxed);
      auto sum = prev + measurement;
      if (m_sum.compare_exchange_weak(prev, sum, boost::memory_order_release,
                                      boost::memory_order_relaxed)) {
        break;
      }
    }
    ++m_size;
    return *this;
  }

 public:
  void Dump(std::ostream &, size_t numberOfSubPeriods) const;

  size_t GetSize() const { return m_size; }

  PeriodFromStart GetMax() const { return m_max; }

  PeriodFromStart GetMin() const { return m_min; }

  PeriodFromStart GetAvg() const { return m_size > 0 ? m_sum / m_size : 0; }

  void Reset() throw() {
    m_min = 0;
    m_max = 0;
    m_sum = 0;
    m_size = 0;
  }

 private:
  boost::atomic<PeriodFromStart> m_min;
  boost::atomic<PeriodFromStart> m_max;
  boost::atomic<PeriodFromStart> m_sum;
  volatile boost::atomic_size_t m_size;
};

////////////////////////////////////////////////////////////////////////////////

template <size_t milestonesCount>
class MilestonesStatAccum : public StatAccum {
 public:
  typedef boost::array<MilestoneStat, milestonesCount> MilestonesStat;

 public:
  virtual ~MilestonesStatAccum() {}

 public:
  virtual void AddMeasurement(const MilestoneIndex &milestone,
                              const PeriodFromStart &period) {
    AssertGt(m_milestones.size(), milestone);
    if (milestone >= m_milestones.size()) {
      return;
    }
    m_milestones[milestone] |= period;
  }

 public:
  bool HasMeasures() const {
    for (const auto &milestone : m_milestones) {
      if (milestone.GetSize()) {
        return true;
      }
    }
    return false;
  }

  const MilestonesStat &GetMilestones() const { return m_milestones; }

  void Reset() {
    for (auto &milestone : m_milestones) {
      milestone.Reset();
    }
  }

 private:
  MilestonesStat m_milestones;
};

////////////////////////////////////////////////////////////////////////////////
}
}
}
