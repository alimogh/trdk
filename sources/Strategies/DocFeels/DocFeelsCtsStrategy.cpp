/*******************************************************************************
 *   Created: 2017/08/20 14:18:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DocFeelsCtsStrategy.hpp"
#include "Core/TradingLog.hpp"
#include "DocFeelsCtsTrend.hpp"
#include "DocFeelsCumulativeReturnFilterService.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::DocFeels;

namespace pt = boost::posix_time;

namespace {
class Generator : private boost::noncopyable {
 public:
  explicit Generator(size_t rangeFrom, size_t rangeTo)
      :
#ifndef DEV_VER
        m_seed(static_cast<uint32_t>(pt::microsec_clock::universal_time()
                                         .time_of_day()
                                         .total_nanoseconds())),
#else
        m_seed(boost::mt19937::default_seed),
#endif
        m_random(m_seed),
        m_range(rangeFrom, rangeTo),
        m_generate(m_random, m_range) {
  }

 public:
  size_t Generate() const { return m_generate(); }

 private:
  const uint32_t m_seed;
  boost::mt19937 m_random;
  boost::uniform_int<size_t> m_range;
  mutable boost::variate_generator<boost::mt19937, boost::uniform_int<size_t>>
      m_generate;
};
}

CtsStrategy::CtsStrategy(Context &context,
                         const std::string &instanceName,
                         const IniSectionRef &conf,
                         const boost::shared_ptr<CtsTrend> &trend)
    : Base(context,
           "{afbc3244-cc9c-4aa0-bc94-223f9232f043}",
           "DocFeelsCts",
           instanceName,
           conf,
           trend ? trend : boost::make_shared<CtsTrend>(conf)) {
  const auto numberOfCts1 = conf.ReadTypedKey<size_t>("number_of_cts1");
  const auto numberOfRtfsPerGroup =
      conf.ReadTypedKey<size_t>("number_of_rtfs_per_group");
  const auto numberOfRtfGroups =
      conf.ReadTypedKey<size_t>("number_of_rtf_groups");
  for (size_t i = 0; i < numberOfCts1; ++i) {
    boost::format ctsInstanceName("CTS-1.%1%");
    ctsInstanceName % (i + 1);
    m_cts1.emplace_back(std::make_unique<CumulativeReturnFilterService>(
        context, ctsInstanceName.str(), conf));
  }
  if (!m_cts1.empty()) {
    const Generator generator(0, m_cts1.size() - 1);
    for (size_t groupIndex = 0; groupIndex < numberOfRtfGroups; ++groupIndex) {
      std::vector<Group::Rtf> group;
      group.reserve(numberOfRtfsPerGroup);
      for (size_t rtfInGroupIndex = 0; rtfInGroupIndex < numberOfRtfsPerGroup;
           ++rtfInGroupIndex) {
        group.emplace_back(Group::Rtf{generator.Generate()});
      }
      m_groups.emplace_back(Group{std::move(group)});
    }
  }
}

CtsStrategy::~CtsStrategy() {
  try {
    PrintStat();
  } catch (...) {
    AssertFailNoException();
  }
}

void CtsStrategy::CheckSignal(const Milestones &delayMeasurement) {
  m_lastDataTime = GetBarService().GetLastDataTime();
  if (m_firstDataTime.is_not_a_date_time()) {
    m_firstDataTime = m_lastDataTime;
  }

  size_t numberOfResults = 0;
  for (auto &cts1 : m_cts1) {
    if (cts1->RaiseServiceDataUpdateEvent(GetBarService(), delayMeasurement)) {
      ++numberOfResults;
    }
  }
  if (!numberOfResults) {
    return;
  }
  AssertEq(m_cts1.size(), numberOfResults);

  intmax_t confluence = 0;
  if (numberOfResults) {
    for (auto &group : m_groups) {
      intmax_t groupConfluence = 0;
      for (auto &rtf : group.rtfs) {
        const auto &rtfValue =
            m_cts1[rtf.index]->GetLastPoint().bestTNewValues.nprtf;
        ++(rtfValue <= 0 ? rtf.numberOfLosses : rtf.numberOfWins);
        groupConfluence += rtfValue;
      }
      ++(groupConfluence <= 0 ? group.numberOfTotalLosses
                              : group.numberOfTotalWins);
      confluence += groupConfluence;
    }
  }

  const bool isTrendChanged = GetTrend().Update(confluence);
  GetTradingLog().Write(
      "trend\t%1%\t%2%\tconfluence=%3%", [&](TradingRecord &record) {
        record % (isTrendChanged ? "CHANGED" : "not-changed")  // 1
            % (GetTrend().IsRising()
                   ? "rising"
                   : GetTrend().IsFalling() ? "falling" : "     ")  // 2
            % confluence;                                           // 3
      });
  if (isTrendChanged) {
    OnSignal(delayMeasurement);
  }
}

namespace {
void PrintStatHeadTime(std::ostream &os, const pt::ptime &time) {
  const auto &date = time.date();
  os << date.day() << '-' << date.month() << '-' << (date.year() - 2000);
}
void PrintStatHead(std::ostream &os,
                   const pt::ptime &startTime,
                   const pt::ptime &endTime,
                   const char *name) {
  os << "Period: ";
  PrintStatHeadTime(os, startTime);
  os << " to ";
  PrintStatHeadTime(os, endTime);
  os << std::endl;
  os << ',' << name << std::endl;
}
}

void CtsStrategy::PrintStat() const {
  {
    std::ofstream groupsReport = OpenDataLog("csv", "_GROUP");
    std::ofstream rtfsReport = OpenDataLog("csv", "_RTFS");
    PrintStatHead(groupsReport, m_firstDataTime, m_lastDataTime, "Avg. %win");
    PrintStatHead(rtfsReport, m_firstDataTime, m_lastDataTime, "Avg. %win");
    size_t groupIndex = 1;
    for (auto &group : m_groups) {
      groupsReport << "Group.#" << groupIndex << ','
                   << static_cast<intmax_t>(
                          ((static_cast<double>(group.numberOfTotalWins) /
                            (group.numberOfTotalWins +
                             group.numberOfTotalLosses))) *
                          100)
                   << std::endl;
      size_t rtfIndex = 1;
      for (auto &rtf : group.rtfs) {
        rtfsReport << "RTF.Group" << groupIndex << ".#" << rtfIndex << ','
                   << static_cast<intmax_t>(
                          (static_cast<double>(rtf.numberOfWins) /
                           (rtf.numberOfWins + rtf.numberOfLosses)) *
                          100)
                   << std::endl;
        ++rtfIndex;
      }
      ++groupIndex;
    }
  }
  {
    std::ofstream cts2Report = OpenDataLog("csv", "_CTS_II");
    PrintStatHead(cts2Report, m_firstDataTime, m_lastDataTime,
                  "Avg. (%return per trade),Avg. (%return per week),Avg. loss "
                  "trade,Avg. win trade,Avg. (No. of trades/wk),Avg. (No. of "
                  "trades/wk),Avg. %win");
  }
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::Strategy> CreateCtsStrategy(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf) {
  return boost::make_shared<CtsStrategy>(context, instanceName, conf);
}

////////////////////////////////////////////////////////////////////////////////
