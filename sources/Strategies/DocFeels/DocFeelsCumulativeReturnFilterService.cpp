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

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::DocFeels;

namespace pt = boost::posix_time;

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

  const size_t m_size;

  std::ofstream m_tOldLog;
  std::ofstream m_tNewLog;

  Point m_data;

  std::unique_ptr<CumulativeReturnService> m_source;

 public:
  explicit Implementation(CumulativeReturnFilterService &self,
                          const IniSectionRef &conf)
      : m_self(self), m_size(conf.ReadTypedKey<size_t>("size")) {
    const bool isLogEnabled = conf.ReadBoolKey("log");
    m_self.GetLog().Info("Size: %1%. Log: %2%.",
                         m_size,                        // 1
                         isLogEnabled ? "yes" : "no");  // 2
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
    log << "Date"      // 1
        << ",Time"     // 2
        << ",Close"    // 3
        << ",%Change"  // 4
        << ",t";       // 5
    for (size_t i = 1; i <= m_size; ++i) {
      log << ',' << i << ".RT," << i << ".CR," << i << ".#";
    }
    log << std::endl;
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
                 const std::vector<Point::Value> &values,
                 bool isNew,
                 std::ostream &log) {
    log << point.time.date()                                // 1
        << ',' << ExcelTextField(point.time.time_of_day())  // 2
        << ',' << point.source                              // 3
        << ',' << point.change                              // 4
        << ',' << (!isNew ? point.tOld : point.tNew);       // 5
    for (const auto &val : values) {
      log << ',' << val.rt << ',' << val.crPeriod << ',' << val.number;
    }
    log << std::endl;
  }

  void Update(const CumulativeReturnService::Point &source) {
    std::vector<Index> sortTOld;
    sortTOld.reserve(source.branches.size());
    std::vector<Index> sortTNew;
    sortTNew.reserve(source.branches.size());
    {
      size_t index = 0;
      for (const auto &branch : source.branches) {
        sortTOld.emplace_back(Index{branch.tOld.crPeriod, index});
        sortTNew.emplace_back(Index{branch.tNew.crPeriod, index});
        ++index;
      }
    }

    SetDate(sortTOld, source, false, m_data.bestTOldValues);
    SetDate(sortTNew, source, true, m_data.bestTNewValues);

    m_data.time = source.time;
    m_data.source = source.source;
    m_data.change = source.change;
    m_data.tOld = source.tOld;
    m_data.tNew = source.tNew;

    LogPoint(m_data);
  }

  void SetDate(std::vector<Index> &sort,
               const CumulativeReturnService::Point &source,
               bool isNew,
               std::vector<Point::Value> &result) {
    std::sort(sort.begin(), sort.end(), std::greater<Index>());

    const auto size = std::min(source.branches.size(), m_size);
    result.clear();
    result.reserve(size);

    for (size_t i = 0; i < size; ++i) {
      const auto &index = sort[i].index;
      const auto &point = source.branches[index];
      const auto &tPoint = isNew ? point.tNew : point.tOld;
      AssertEq(sort[i].crPeriod, tPoint.crPeriod);
      result.emplace_back(Point::Value{index + 1, point.r, tPoint.rt, tPoint.cr,
                                       tPoint.crPeriod});
    }
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
  Assert(m_pimpl->m_source);
  if (!m_pimpl->m_source->RaiseServiceDataUpdateEvent(service, milestones)) {
    return false;
  }
  const auto *const crService = &*m_pimpl->m_source;
  /*  const auto *const crService =
        dynamic_cast<const CumulativeReturnService *>(&service);
    if (!crService) {
      GetLog().Error("Failed to use service \"%1%\" used as data source.",
                     service);
      throw Exception("Unknown service used as source");
    }*/

  Assert(!crService->IsEmpty());
  m_pimpl->Update(crService->GetLastPoint());
  Assert(!IsEmpty());
  return true;
}

void CumulativeReturnFilterService::OnServiceStart(const Service &service) {
  Assert(!m_pimpl->m_source);
  std::string settingsString(
      "[Section]\n"
      "id = 98C7DD36-28CC-4202-A103-0352BF0B2066\n"
      "cr_period = 30\n"
      "size_of_rt_set = 100\n"
      "log = yes\n");
  const IniString settings(settingsString);
  m_pimpl->m_source = boost::make_unique<CumulativeReturnService>(
      GetContext(), "CumulativeReturn", IniSectionRef(settings, "Section"));
  m_pimpl->m_source->OnServiceStart(service);
}
