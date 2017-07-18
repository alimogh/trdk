/**************************************************************************
 *   Created: 2017/01/04 01:31:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "AdxIndicator.hpp"
#include "BarService.hpp"
#include "Common/Accumulators.hpp"

namespace pt = boost::posix_time;
namespace uuids = boost::uuids;
namespace accs = boost::accumulators;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Accumulators;
using namespace trdk::Services::Indicators;

////////////////////////////////////////////////////////////////////////////////

namespace boost {
namespace accumulators {

namespace impl {

template <typename Sample>
struct DirectionalMovement : public accumulator_base {
  typedef Sample result_type;

  template <typename Args>
  explicit DirectionalMovement(const Args &args)
      : m_windowSize(args[rolling_window_size]), m_result(0) {}

  template <typename Args>
  void operator()(const Args &args) {
    if (count(args) <= m_windowSize) {
      m_result += args[sample];
      return;
    }
    m_result = m_result - (m_result / m_windowSize) + args[sample];
  }

  const result_type &result(const dont_care &) const { return m_result; }

 private:
  const size_t m_windowSize;
  result_type m_result;
};
}

namespace tag {
struct DirectionalMovement : public depends_on<count> {
  typedef accumulators::impl::DirectionalMovement<mpl::_1> impl;
};
}

namespace extract {
const extractor<tag::DirectionalMovement> directionalMovement = {

};
BOOST_ACCUMULATORS_IGNORE_GLOBAL(directionalMovement)
}

using extract::directionalMovement;
}
}

namespace {

typedef boost::accumulators::accumulator_set<
    double,
    boost::accumulators::stats<boost::accumulators::tag::DirectionalMovement>>
    DirectionalMovementAcc;
}

////////////////////////////////////////////////////////////////////////////////

class Adx::Implementation : private boost::noncopyable {
 public:
  Adx &m_self;

  const size_t m_period;

  Point m_lastValue;
  size_t m_lastValueNo;

  DirectionalMovementAcc m_trEma;
  DirectionalMovementAcc m_pdmEma;
  DirectionalMovementAcc m_ndmEma;
  accs::accumulator_set<
      Double,
      accs::stats<accs::tag::MovingAverageSmoothed, accs::tag::count>>
      m_dxiMa;

  std::ofstream m_pointsLog;

  explicit Implementation(Adx &self, const IniSectionRef &conf)
      : m_self(self),
        m_period(conf.ReadTypedKey<size_t>("period")),
        m_lastValue(Point{}),
        m_lastValueNo(0),
        m_trEma(accs::tag::rolling_window::window_size = m_period),
        m_pdmEma(accs::tag::rolling_window::window_size = m_period),
        m_ndmEma(accs::tag::rolling_window::window_size = m_period),
        m_dxiMa(accs::tag::rolling_window::window_size = m_period) {
    if (m_period < 2) {
      m_self.GetLog().Error(
          "Wrong period (%1% frames): must be greater than 1.", m_period);
      throw Exception("Wrong ADX period");
    }

    m_self.GetLog().Info("Using period %1%.", m_period);

    {
      const std::string logType = conf.ReadKey("log");
      if (boost::iequals(logType, "none")) {
        m_self.GetLog().Info("Values logging is disabled.");
      } else if (!boost::iequals(logType, "csv")) {
        m_self.GetLog().Error(
            "Wrong values log type settings: \"%1%\". Unknown type."
            " Supported: none and CSV.",
            logType);
        throw Error("Wrong values log type");
      } else {
        OpenPointsLog();
      }
    }
  }

  void OpenPointsLog() {
    Assert(!m_pointsLog.is_open());
    auto log = m_self.OpenDataLog("csv");
    log << "Date,Time,Open,High,Low,Close," << ExcelTextField("+DI") << ','
        << ExcelTextField("-DI") << ",ADX" << std::endl;
    log << std::setfill('0');
    m_pointsLog = std::move(log);
  }

  void LogEmptyPoint(const Point::Source &source,
                     const boost::optional<Double> &pdi = boost::none,
                     const boost::optional<Double> &ndi = boost::none) {
    if (!m_pointsLog.is_open()) {
      return;
    }
    m_pointsLog << source.time.date() << ','
                << ExcelTextField(source.time.time_of_day()) << ','
                << source.open << ',' << source.high << ',' << source.low << ','
                << source.close;
    m_pointsLog << ',';
    if (pdi) {
      m_pointsLog << *pdi;
    }
    m_pointsLog << ',';
    if (ndi) {
      m_pointsLog << *ndi;
    }
    m_pointsLog << ',' << std::endl;
  }

  void LogPoint(const Point &point) {
    if (!m_pointsLog.is_open()) {
      return;
    }
    m_pointsLog << point.source.time.date() << ','
                << ExcelTextField(point.source.time.time_of_day()) << ','
                << point.source.open << ',' << point.source.high << ','
                << point.source.low << ',' << point.source.close << ','
                << point.pdi << ',' << point.ndi << ',' << point.adx
                << std::endl;
  }

  bool OnNewValue(const BarService::Bar &bar, const Security &security) {
    {
      Point point = {{
          bar.time, security.DescalePrice(bar.openTradePrice),
          security.DescalePrice(bar.highTradePrice),
          security.DescalePrice(bar.lowTradePrice),
          security.DescalePrice(bar.closeTradePrice),
      }};

      Assert(point.source.time != pt::not_a_date_time);
      if (m_lastValue.source.time == pt::not_a_date_time) {
        AssertEq(0, m_lastValueNo);
        m_trEma(0);
        m_pdmEma(0);
        m_ndmEma(0);
        LogEmptyPoint(point.source);
        m_lastValue = std::move(point);
        return false;
      }

      {
        const auto trueRange = std::max<Double>(
            point.source.high - point.source.low,
            std::max<Double>(
                std::abs(point.source.high - m_lastValue.source.close),
                std::abs(point.source.low - m_lastValue.source.close)));
        m_trEma(trueRange);
      }

      {
        //! Positive movement.
        const Double pm = point.source.high - m_lastValue.source.high;
        //! Negative movement
        const Double nm = m_lastValue.source.low - point.source.low;

        //! Positive directional movement.
        const auto pdm = pm > nm && pm > 0 ? pm : 0;
        //! Negative directional movement.
        const auto ndm = nm > pm && nm > 0 ? nm : 0;

        m_pdmEma(pdm);
        m_ndmEma(ndm);
      }

      m_lastValue = std::move(point);
    }

    AssertEq(accs::count(m_trEma), accs::count(m_pdmEma));
    AssertEq(accs::count(m_pdmEma), accs::count(m_ndmEma));
    if (accs::count(m_trEma) <= m_period) {
      AssertEq(0, m_lastValueNo);
      LogEmptyPoint(m_lastValue.source);
      return false;
    }

    {
      const auto &trStat = accs::directionalMovement(m_trEma);
      m_lastValue.pdi = (accs::directionalMovement(m_pdmEma) / trStat) * 100;
      m_lastValue.ndi = (accs::directionalMovement(m_ndmEma) / trStat) * 100;
    }

    {
      //! Directional movement index.
      auto dxi = std::abs(m_lastValue.pdi - m_lastValue.ndi);
      dxi /= m_lastValue.pdi + m_lastValue.ndi;
      dxi *= 100;
      m_dxiMa(dxi);

      if (accs::count(m_dxiMa) <= m_period) {
        AssertEq(0, m_lastValueNo);
        LogEmptyPoint(m_lastValue.source, m_lastValue.pdi, m_lastValue.ndi);
        return false;
      }
    }

    m_lastValue.adx = accs::smma(m_dxiMa);

    LogPoint(m_lastValue);
    ++m_lastValueNo;

    return true;
  }
};

////////////////////////////////////////////////////////////////////////////////

Adx::Error::Error(const char *what) noexcept : Exception(what) {}

Adx::ValueDoesNotExistError::ValueDoesNotExistError(const char *what) noexcept
    : Error(what) {}

////////////////////////////////////////////////////////////////////////////////

Adx::Adx(Context &context,
         const std::string &instanceName,
         const IniSectionRef &conf)
    : Service(
          context,
          uuids::string_generator()("{e15e8918-c7a9-4875-a02f-c80f8ab4e90c}"),
          "AdxIndicator",
          instanceName,
          conf),
      m_pimpl(boost::make_unique<Implementation>(*this, conf)) {}

Adx::~Adx() noexcept {}

const pt::ptime &Adx::GetLastDataTime() const {
  return GetLastPoint().source.time;
}

bool Adx::IsEmpty() const { return m_pimpl->m_lastValueNo == 0; }

const Adx::Point &Adx::GetLastPoint() const {
  if (IsEmpty()) {
    throw ValueDoesNotExistError("ADX Indicator is empty");
  }
  return m_pimpl->m_lastValue;
}

bool Adx::OnServiceDataUpdate(const Service &service,
                              const TimeMeasurement::Milestones &) {
  const auto &barService = dynamic_cast<const BarService *>(&service);
  if (!barService) {
    GetLog().Error("Failed to use service \"%1%\" as data source.", service);
    throw Error("Unknown service used as source");
  }

  return m_pimpl->OnNewValue(barService->GetLastBar(),
                             barService->GetSecurity());
}

////////////////////////////////////////////////////////////////////////////////

TRDK_SERVICES_API boost::shared_ptr<trdk::Service> CreateAdxIndicatorService(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<Adx>(context, instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
