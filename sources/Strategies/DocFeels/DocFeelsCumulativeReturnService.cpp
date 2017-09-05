/*******************************************************************************
 *   Created: 2017/09/03 14:47:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DocFeelsCumulativeReturnService.hpp"
#include "Services/BarService.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::DocFeels;

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;

////////////////////////////////////////////////////////////////////////////////

namespace {
class RandomRGenerator : public CumulativeReturnService::RGenerator {
 public:
  RandomRGenerator()
      :
#ifndef DEV_VER
        m_seed(static_cast<uint32_t>(pt::microsec_clock::universal_time()
                                         .time_of_day()
                                         .total_nanoseconds())),
#else
        m_seed(boost::mt19937::default_seed),
#endif
        m_random(m_seed),
        m_range(0, 1),
        m_generate(m_random, m_range) {
  }
  virtual ~RandomRGenerator() override = default;

 public:
  virtual CumulativeReturnService::Point::R Generate() const override {
    return m_generate() ? 1 : -1;
  }

  virtual uint32_t GetParam() const override { return m_seed; }

 private:
  const uint32_t m_seed;
  boost::mt19937 m_random;
  boost::uniform_int<uint8_t> m_range;
  mutable boost::variate_generator<boost::mt19937, boost::uniform_int<uint8_t>>
      m_generate;
};
}

////////////////////////////////////////////////////////////////////////////////

class CumulativeReturnService::Implementation : private boost::noncopyable {
 public:
  struct Stat {
    struct T {
      accs::accumulator_set<Double, accs::stats<accs::tag::sum>> cr;
      accs::accumulator_set<Double, accs::stats<accs::tag::rolling_sum>>
          crPeriod;

      explicit T(size_t period)
          : crPeriod(accs::tag::rolling_window::window_size = period) {}
    };

    T tNew;
    T tOld;

    explicit Stat(size_t period) : tNew(period), tOld(period) {}
  };

 public:
  CumulativeReturnService &m_self;

  const size_t m_numberOfCpPeriods;
  size_t m_statSize;
  std::vector<Stat> m_crStat;

  Price m_startPrice;
  boost::optional<Point> m_lastValue;
  accs::accumulator_set<Double, accs::stats<accs::tag::sum>> m_tOldStat;

  const boost::shared_ptr<RGenerator> m_rGenerator;

  std::ofstream m_tOldLog;
  std::ofstream m_tNewLog;

  Implementation(CumulativeReturnService &self,
                 const IniSectionRef &conf,
                 const boost::shared_ptr<RGenerator> &rGenerator)
      : m_self(self),
        m_numberOfCpPeriods(conf.ReadTypedKey<size_t>("cr_period")),
        m_statSize(0),
        m_rGenerator(rGenerator ? rGenerator
                                : boost::make_shared<RandomRGenerator>()) {
    const auto &sizeOfRtSet = conf.ReadTypedKey<size_t>("size_of_rt_set");
    const bool isLogEnabled = conf.ReadBoolKey("log");
    m_self.GetLog().Info(
        "Size of RT set: %1%. Number of CR periods: %2%. Log: %3%. R-generator "
        "param: %4%.",
        sizeOfRtSet,                  // 1
        m_numberOfCpPeriods,          // 2
        isLogEnabled ? "yes" : "no",  // 3
        m_rGenerator->GetParam());    // 4

    if (isLogEnabled && sizeOfRtSet > 100) {
      throw Exception(
          "Size of RT set is too big for logging. Disabled logging or reduce "
          "RT set size.");
    }

    for (size_t i = 0; i < sizeOfRtSet; ++i) {
      m_crStat.emplace_back(m_numberOfCpPeriods);
    }

    if (isLogEnabled) {
      OpenPointsLog();
    }
  }

  void OpenPointsLog() {
    Assert(!m_tOldLog.is_open());
    Assert(!m_tNewLog.is_open());
    auto tOldLog = m_self.OpenDataLog("csv", "_tOld");
    auto tNewLog = m_self.OpenDataLog("csv", "_tNew");
    OpenTLog(tOldLog);
    OpenTLog(tNewLog);
    m_tOldLog = std::move(tOldLog);
    m_tNewLog = std::move(tNewLog);
  }

  void OpenTLog(std::ostream &log) {
    log << "Date"      // 1
        << ",Time"     // 2
        << ",Close"    // 3
        << ",%Change"  // 4
        << ",t";       // 5
    for (size_t i = 1; i <= m_crStat.size(); ++i) {
      log << ",R." << i;
    }
    for (size_t i = 1; i <= m_crStat.size(); ++i) {
      log << ",RT." << i;
    }
    for (size_t i = 1; i <= m_crStat.size(); ++i) {
      log << ",CR(RT." << i << ')';
    }
    for (size_t i = 1; i <= m_crStat.size(); ++i) {
      log << ',' << m_numberOfCpPeriods << " - CR(RT." << i << ')';
    }
    log << std::endl;
    log << std::setfill('0');
  }

  void LogEmptyPoint(const Point &point) {
    if (!m_tOldLog.is_open()) {
      return;
    }
    LogTEmpty(point, m_tOldLog);
    LogTEmpty(point, m_tNewLog);
  }
  void LogTEmpty(const Point &point, std::ostream &log) {
    log << point.time.date()                                // 1
        << ',' << ExcelTextField(point.time.time_of_day())  // 2
        << ',' << point.source                              // 3
        << std::endl;
  }

  void LogPoint(const Point &point) {
    if (!m_tOldLog.is_open()) {
      return;
    }
    LogTPoint(point, false, m_tOldLog);
    LogTPoint(point, true, m_tNewLog);
  }
  void LogTPoint(const Point &point, bool isNew, std::ostream &log) {
    log << point.time.date()                                // 1
        << ',' << ExcelTextField(point.time.time_of_day())  // 2
        << ',' << point.source                              // 3
        << ',' << point.change                              // 4
        << ',' << (!isNew ? point.tOld : point.tNew);       // 5
    for (const auto &branch : point.branches) {
      log << ',' << branch.r;
    }
    for (const auto &branch : point.branches) {
      log << ',' << (!isNew ? branch.tOld.rt : branch.tNew.rt);
    }
    for (const auto &branch : point.branches) {
      log << ',' << (!isNew ? branch.tOld.cr : branch.tNew.cr);
    }
    for (const auto &branch : point.branches) {
      log << ',' << (!isNew ? branch.tOld.crPeriod : branch.tNew.crPeriod);
    }
    log << std::endl;
  }

  void OnPriceUpdate(const pt::ptime &time, const Price &price) {
    AssertLt(0, price);
    if (price == 0) {
      return;
    }
    if (!m_lastValue) {
      AssertEq(0, m_statSize);
      m_startPrice = price;
      m_lastValue = Point{time, price};
      m_lastValue->branches.resize(m_crStat.size());
      LogEmptyPoint(*m_lastValue);
      return;
    }

    auto &result = *m_lastValue;

    result.time = time;
    result.change = RoundByScale(((price / result.source) - 1) * 100, 100);
    m_tOldStat(result.change);
    result.source = price;
    result.tOld = RoundByScale(accs::sum(m_tOldStat), 100);
    result.tNew =
        RoundByScale(((price - m_startPrice) / m_startPrice) * 100, 100);

    {
      size_t index = 0;
      for (auto &branch : result.branches) {
        branch.r = m_rGenerator->Generate();
        CalculateBranch(index, true, result.tNew, branch.r, branch.tNew);
        CalculateBranch(index, false, result.tOld, branch.r, branch.tOld);
        ++index;
        AssertLe(index, result.branches.size());
        AssertLe(index, m_crStat.size());
      }
    }

    ++m_statSize;

    LogPoint(result);
  }

  void CalculateBranch(size_t index,
                       bool isTNew,
                       const Double &t,
                       const Point::R &r,
                       Point::Branch::T &result) {
    AssertLt(index, m_crStat.size());
    auto &stat = isTNew ? m_crStat[index].tNew : m_crStat[index].tOld;
    result.rt = t * r;
    stat.cr(result.rt);
    result.cr = accs::sum(stat.cr);
    result.crPeriod = accs::rolling_sum(stat.crPeriod);
    stat.crPeriod(result.cr);
  }
};

CumulativeReturnService::CumulativeReturnService(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf,
    const boost::shared_ptr<RGenerator> &rGenerator)
    : Service(context,
              "{063643A2-0F58-45D4-BCAC-467076F7B6DC}",
              "DocFeelsCumulativeReturnService",
              instanceName,
              conf),
      m_pimpl(boost::make_unique<Implementation>(*this, conf, rGenerator)) {}

CumulativeReturnService::~CumulativeReturnService() {}

pt::ptime CumulativeReturnService::GetLastDataTime() const {
  return GetLastPoint().time;
}

bool CumulativeReturnService::IsEmpty() const {
  return m_pimpl->m_statSize <= m_pimpl->m_numberOfCpPeriods;
}

const CumulativeReturnService::Point &CumulativeReturnService::GetLastPoint()
    const {
  if (IsEmpty()) {
    throw Exception("Cumulative Return Service is empty");
  }
  Assert(m_pimpl->m_lastValue);
  return *m_pimpl->m_lastValue;
}

bool CumulativeReturnService::OnServiceDataUpdate(
    const Service &service, const TimeMeasurement::Milestones &) {
  const auto *const barService =
      dynamic_cast<const Services::BarService *>(&service);
  if (!barService) {
    GetLog().Error("Failed to use service \"%1%\" used as data source.",
                   service);
    throw Exception("Unknown service used as source");
  }

  AssertLt(0, barService->GetSize());
  const auto &bar = barService->GetLastBar();
  m_pimpl->OnPriceUpdate(
      bar.time, barService->GetSecurity().DescalePrice(bar.closeTradePrice));
  return !IsEmpty();
}
