/*******************************************************************************
 *   Created: 2017/09/05 18:51:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DocFeelsCumulativeReturnFilterService.hpp"
#include "Services/BarService.hpp"
#include "Services/BollingerBandsService.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::DocFeels;

namespace pt = boost::posix_time;
namespace accs = boost::accumulators;

namespace {
class Indicator : private boost::noncopyable {
 public:
  typedef CumulativeReturnService::Point::R R;

 public:
  virtual ~Indicator() = default;

 public:
  virtual void Update(const pt::ptime &, const Double &) = 0;
  virtual Double GetState() const = 0;
  virtual R GetSignal(const Double &crPeriod) const = 0;
};
class SmaIndicator : public Indicator {
 public:
  SmaIndicator(const IniSectionRef &conf)
      : m_acc(accs::tag::rolling_window::window_size =
                  conf.ReadTypedKey<size_t>("cr_period")) {}
  virtual ~SmaIndicator() override = default;

 public:
  virtual void Update(const pt::ptime &, const Double &value) override {
    m_acc(value);
  }
  virtual Double GetState() const override { return accs::rolling_mean(m_acc); }
  virtual R GetSignal(const Double &crPeriod) const override {
    return crPeriod > accs::rolling_mean(m_acc) ? 1 : -1;
  }

 private:
  accs::accumulator_set<Double, accs::stats<accs::tag::rolling_mean>> m_acc;
};
class BbIndicator : public Indicator {
 public:
  explicit BbIndicator(Context &context, const IniSectionRef &conf) {
    boost::format bbSettingsSource(
        "[Section]\n"
        "id = {00000000-0000-0000-0000-000000000000}\n"
        "id.low = {00000000-0000-0000-0000-000000000000}\n"
        "id.high = {00000000-0000-0000-0000-000000000000}\n"
        "period = %1%\n"
        "deviation = 2.0\n"
        "history = no\n"
        "log = none\n");
    bbSettingsSource % conf.ReadTypedKey<size_t>("cr_period");
    const IniString bbSettings(bbSettingsSource.str());
    m_acc = boost::make_unique<Services::BollingerBandsService>(
        context, "BbIndicator", IniSectionRef(bbSettings, "Section"));
  }
  virtual ~BbIndicator() override = default;

 public:
  virtual void Update(const pt::ptime &time, const Double &value) override {
    m_acc->Update(time, value);
  }
  virtual Double GetState() const override {
    return m_acc->IsEmpty() ? 0 : m_acc->GetLastPoint().middle;
  }
  virtual R GetSignal(const Double &crPeriod) const override {
    if (m_acc->IsEmpty()) {
      return 0;
    }
    const auto &bb = m_acc->GetLastPoint();
    return crPeriod > bb.high ? 1 : crPeriod < bb.low ? -1 : 0;
  }

 private:
  std::unique_ptr<Services::BollingerBandsService> m_acc;
};
}

class CumulativeReturnFilterService::Implementation
    : private boost::noncopyable {
 public:
  struct Index {
    trdk::Lib::Double crPeriod;
    size_t index;
    bool operator>(const Index &rhs) const { return crPeriod > rhs.crPeriod; }
  };

 public:
  CumulativeReturnFilterService &m_self;

  std::ofstream m_tOldLog;
  std::ofstream m_tNewLog;

  Point m_data;

  CumulativeReturnService m_source;

  std::unique_ptr<Indicator> m_indicatorTOld;
  std::unique_ptr<Indicator> m_indicatorTNew;

 public:
  explicit Implementation(CumulativeReturnFilterService &self,
                          const IniSectionRef &conf)
      : m_self(self), m_source(m_self.GetContext(), "CumulativeReturn", conf) {
    {
      const auto &indicator = conf.ReadKey("cts1_idicator");
      if (boost::iequals(indicator, "sma")) {
        m_indicatorTOld = boost::make_unique<SmaIndicator>(conf);
        m_indicatorTNew = boost::make_unique<SmaIndicator>(conf);
      } else if (boost::iequals(indicator, "bb")) {
        m_indicatorTOld =
            boost::make_unique<BbIndicator>(m_self.GetContext(), conf);
        m_indicatorTNew =
            boost::make_unique<BbIndicator>(m_self.GetContext(), conf);
      } else {
        m_self.GetLog().Error("Unknown indicator: \"%1%\".", indicator);
        throw Exception("Unknown indicator");
      }
    }

    const bool isLogEnabled = conf.ReadBoolKey("cts1_nprtf_log");
    m_self.GetLog().Info("Log: %1%.", isLogEnabled ? "yes" : "no");
    if (isLogEnabled) {
      OpenPointsLog();
    }
  }

  void OpenPointsLog() {
    Assert(!m_tOldLog.is_open());
    Assert(!m_tNewLog.is_open());
    auto tOldLog = m_self.OpenDataLog("csv", "_tOld");
    auto tNewLog = m_self.OpenDataLog("csv", "_tNew");
    PreparePointsLog(tOldLog);
    PreparePointsLog(tNewLog);
    m_tOldLog = std::move(tOldLog);
    m_tNewLog = std::move(tNewLog);
  }

  void PreparePointsLog(std::ostream &log) {
    auto tOldLog = m_self.OpenDataLog("csv", "_tOld");
    log << "Date"            // 1
        << ",Time"           // 2
        << ",Close"          // 3
        << ",%Change"        // 4
        << ",t"              // 5
        << ",#"              // 6
        << ",R"              // 7
        << ",CR"             // 8
        << ",Indicator(CR)"  // 9
        << ",CR Period"      // 10
        << ",Signal"         // 11
        << ",NPRTF"          // 12
        << std::endl;
    log << std::setfill('0');
  }

  void LogPoint(const Point &point) {
    if (!m_tOldLog.is_open()) {
      return;
    }
    LogTPoint(point, point.bestTOldValues, false, m_tOldLog);
    LogTPoint(point, point.bestTNewValues, true, m_tNewLog);
  }

  void LogTPoint(const Point &point,
                 const Point::Value &values,
                 bool isNew,
                 std::ostream &log) {
    log << point.time.date()                                // 1
        << ',' << ExcelTextField(point.time.time_of_day())  // 2
        << ',' << point.source                              // 3
        << ',' << point.change                              // 4
        << ',' << (!isNew ? point.tOld : point.tNew)        // 5
        << ',' << values.number                             // 6
        << ',' << values.r                                  // 7
        << ',' << values.cr                                 // 8
        << ',' << values.crIndicator                        // 9
        << ',' << values.crPeriod                           // 10
        << ',' << values.signal                             // 11
        << ',' << values.nprtf                              // 12
        << std::endl;
  }

  void Update(const pt::ptime &time,
              const CumulativeReturnService::Point &source) {
    size_t bestTOldIndex = 0;
    size_t bestTNewIndex = 0;
    for (size_t i = 1; i < source.branches.size(); ++i) {
      const auto &branch = source.branches[i];
      {
        const auto &bestBranch = source.branches[bestTOldIndex].tOld;
        if (bestBranch.crPeriod < branch.tOld.crPeriod) {
          bestTOldIndex = i;
        }
      }
      {
        const auto &bestBranch = source.branches[bestTNewIndex].tOld;
        if (bestBranch.crPeriod < branch.tNew.crPeriod) {
          bestTNewIndex = i;
        }
      }
    }

    SetDate(time, source, bestTOldIndex, false, *m_indicatorTOld,
            m_data.bestTOldValues);
    SetDate(time, source, bestTNewIndex, true, *m_indicatorTNew,
            m_data.bestTNewValues);

    m_data.time = source.time;
    m_data.source = source.source;
    m_data.change = source.change;
    m_data.tOld = source.tOld;
    m_data.tNew = source.tNew;

    LogPoint(m_data);
  }

  void SetDate(const pt::ptime &time,
               const CumulativeReturnService::Point &source,
               size_t index,
               bool isNew,
               Indicator &indicator,
               Point::Value &result) {
    const auto &branch = source.branches[index];
    const auto &sourcePoint = isNew ? branch.tNew : branch.tOld;
    const auto &crIdicator = indicator.GetState();
    const R &signal = indicator.GetSignal(sourcePoint.crPeriod);
    result = Point::Value{index + 1,
                          branch.r,
                          sourcePoint.cr,
                          std::move(crIdicator),
                          sourcePoint.crPeriod,
                          signal,
                          signal * branch.r};
    indicator.Update(time, result.cr);
  }
};

CumulativeReturnFilterService::CumulativeReturnFilterService(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &conf)
    : Service(context,
              "{163E5621-FEAC-417B-97F8-317C57026505}",
              "DocFeelsCumulativeReturnFilterService",
              instanceName,
              conf),
      m_pimpl(boost::make_unique<Implementation>(*this, conf)) {}

CumulativeReturnFilterService::~CumulativeReturnFilterService() {}

pt::ptime CumulativeReturnFilterService::GetLastDataTime() const {
  return GetLastPoint().time;
}

bool CumulativeReturnFilterService::IsEmpty() const {
  return m_pimpl->m_data.time.is_not_a_date_time();
}

const CumulativeReturnFilterService::Point &
CumulativeReturnFilterService::GetLastPoint() const {
  if (IsEmpty()) {
    throw Exception("Cumulative Return Filter Service is empty");
  }
  return m_pimpl->m_data;
}

bool CumulativeReturnFilterService::OnServiceDataUpdate(
    const Service &service, const TimeMeasurement::Milestones &milestones) {
  if (!m_pimpl->m_source.RaiseServiceDataUpdateEvent(service, milestones)) {
    return false;
  }
  const auto *const crService = &m_pimpl->m_source;
  /*  const auto *const crService =
        dynamic_cast<const CumulativeReturnService *>(&service);
    if (!crService) {
      GetLog().Error("Failed to use service \"%1%\" used as data source.",
                     service);
      throw Exception("Unknown service used as source");
    }*/

  Assert(!crService->IsEmpty());
  m_pimpl->Update(crService->GetLastDataTime(), crService->GetLastPoint());
  Assert(!IsEmpty());
  return true;
}

void CumulativeReturnFilterService::OnServiceStart(const Service &service) {
  m_pimpl->m_source.OnServiceStart(service);
}
