/*******************************************************************************
 *   Created: 2017/08/20 14:15:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "DocFeelsStrategy.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class TRDK_STRATEGY_DOCFEELS_API CtsStrategy : public DocFeels::Strategy {
 public:
  typedef DocFeels::Strategy Base;

 private:
  struct Group {
    struct Rtf {
      size_t index;
      size_t numberOfLosses;
      size_t numberOfWins;
    };
    struct Period {
      boost::posix_time::ptime endTime;
      size_t numberOfLosses;
      size_t numberOfWins;
    };
    struct SubPeriod {
      boost::posix_time::ptime startTime;
      size_t numberOfLosses;
      size_t numberOfWins;
    };
    std::vector<Rtf> rtfs;
    size_t numberOfTotalLosses;
    size_t numberOfTotalWins;
  };

  struct Cts2Stat {
    size_t numberOfPositions;
    trdk::Volume pnlsSum;

    size_t numberOfPeriods;
    trdk::Volume periodsPnlSum;
  };

 public:
  explicit CtsStrategy(
      trdk::Context &,
      const std::string &instanceName,
      const trdk::Lib::IniSectionRef &,
      const boost::shared_ptr<CtsTrend> & = boost::shared_ptr<CtsTrend>());
  virtual ~CtsStrategy() override;

 protected:
  CtsTrend &GetTrend() { return GetTypedTrend<CtsTrend>(); }
  const CtsTrend &GetTrend() const { return GetTypedTrend<CtsTrend>(); }

 protected:
  virtual void CheckSignal(const Lib::TimeMeasurement::Milestones &) override;

 private:
  void PrintStat() const;

 private:
  std::vector<std::unique_ptr<CumulativeReturnFilterService>> m_cts1;
  std::vector<Group> m_groups;

  boost::posix_time::ptime m_firstDataTime;
  boost::posix_time::ptime m_lastDataTime;
};
}
}
}