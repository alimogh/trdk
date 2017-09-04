/**************************************************************************
 *   Created: 2012/11/15 23:14:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "BarCollectionService.hpp"
#include "Core/DropCopy.hpp"
#include "Core/Settings.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Services;

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace uuids = boost::uuids;

////////////////////////////////////////////////////////////////////////////////

namespace {

const char csvDelimeter = ',';
}

////////////////////////////////////////////////////////////////////////////////

BarCollectionService::MethodDoesNotSupportBySettings::
    MethodDoesNotSupportBySettings(const char *what) throw()
    : Error(what) {}

//////////////////////////////////////////////////////////////////////////

class BarCollectionService::Implementation : private boost::noncopyable {
 public:
  enum Units {
    UNITS_SECONDS,
    UNITS_MINUTES,
    UNITS_HOURS,
    UNITS_DAYS,
    UNITS_WEEKS,
    UNITS_TICKS,
    numberOfUnits
  };

 public:
  BarService &m_service;

  const Security *m_security;

  std::string m_unitsStr;
  Units m_units;

  size_t m_numberOfHistoryBars;

  std::string m_barSizeStr;
  long m_barSizeUnits;
  pt::time_duration m_timedBarSize;
  size_t m_countedBarSize;

  std::vector<Bar> m_bars;

  struct Current {
    Bar *bar;
    pt::ptime end;
    size_t count;

    Current() noexcept : bar(nullptr), count(0) {}
    explicit Current(Bar &bar) : bar(&bar), count(0) {}

  } m_current;

  std::unique_ptr<std::ofstream> m_barsLog;
  size_t m_session;

  boost::function<bool(const pt::ptime &, bool)> m_isNewBar;

  boost::function<bool(
      const Security &, const pt::ptime &, const Level1TickValue &)>
      m_onLevel1Tick;
  boost::function<bool(const Security &, const Security::Bar &)> m_onNewBar;

 public:
  explicit Implementation(BarService &service,
                          const IniSectionRef &configuration)
      : m_service(service),
        m_security(nullptr),
        m_numberOfHistoryBars(1),
        m_countedBarSize(0),
        m_session(1) {
    {
      const std::string sizeStr = configuration.ReadKey("size");
      const boost::regex expr("(\\d+)\\s+([a-z]+)",
                              boost::regex_constants::icase);
      boost::smatch what;
      if (!boost::regex_match(sizeStr, what, expr)) {
        m_service.GetLog().Error(
            "Wrong size size format: \"%1%\". Example: \"5 minutes\".",
            sizeStr);
        throw Error("Wrong bar size settings");
      }
      m_barSizeStr = what.str(1);
      m_unitsStr = what.str(2);
    }

    m_barSizeUnits =
        boost::lexical_cast<decltype(m_barSizeUnits)>(m_barSizeStr);
    if (m_barSizeUnits <= 0) {
      m_service.GetLog().Error(
          "Wrong size specified: \"%1%\"."
          " Size can't be zero or less.",
          m_barSizeStr);
      throw Error("Wrong bar size settings");
    }

    static_assert(numberOfUnits == 6, "Units list changed.");
    if (boost::iequals(m_unitsStr, "seconds")) {
      if (m_barSizeUnits < 5) {
        // reqRealTimeBars: Currently only 5 second bars are supported,
        // if any other value is used, an exception will be thrown.
        m_service.GetLog().Error(
            "Wrong size specified: \"%1%\"."
            " Can't be less then 5 seconds (IB TWS limitation).",
            m_barSizeStr);
        throw Error("Wrong bar size settings");
      } else if (m_barSizeUnits % 5) {
        // reqRealTimeBars: Currently only 5 second bars are supported,
        // if any other value is used, an exception will be thrown.
        m_service.GetLog().Error(
            "Wrong size specified: \"%1%\"."
            " Must be a multiple of 5 seconds (IB TWS limitation).",
            m_barSizeStr);
        throw Error("Wrong bar size settings");
      } else if (60 % m_barSizeUnits) {
        m_service.GetLog().Error(
            "Wrong size specified: \"%1%\"."
            " Minute must be a multiple of the bar size.",
            m_barSizeStr);
        throw Error("Wrong bar size settings");
      }
      m_units = UNITS_SECONDS;
      m_timedBarSize = pt::seconds(m_barSizeUnits);
    } else if (boost::iequals(m_unitsStr, "minutes")) {
      if (60 % m_barSizeUnits) {
        m_service.GetLog().Error(
            "Wrong size specified: \"%1%\"."
            " Hour must be a multiple of the bar size.",
            m_barSizeStr);
        throw Error("Wrong bar size settings");
      }
      m_units = UNITS_MINUTES;
      m_timedBarSize = pt::minutes(m_barSizeUnits);
    } else if (boost::iequals(m_unitsStr, "hours")) {
      m_units = UNITS_HOURS;
      m_timedBarSize = pt::hours(m_barSizeUnits);
    } else if (boost::iequals(m_unitsStr, "days")) {
      m_units = UNITS_DAYS;
      m_timedBarSize = pt::hours(m_barSizeUnits * 24);
      throw Error("Days units is not implemented");
    } else if (boost::iequals(m_unitsStr, "weeks")) {
      m_units = UNITS_WEEKS;
      m_timedBarSize = pt::hours((m_barSizeUnits * 24) * 7);
      throw Error("Weeks units is not implemented");
    } else if (boost::iequals(m_unitsStr, "ticks")) {
      m_units = UNITS_TICKS;
      m_countedBarSize = m_barSizeUnits;
    } else {
      m_service.GetLog().Error(
          "Wrong size specified: \"%1%\". Unknown units."
          " Supported: seconds, minutes, hours, days, weeks"
          " and ticks.",
          m_unitsStr);
      throw Error("Wrong bar size settings");
    }

    {
      const std::string logType = configuration.ReadKey("log");
      if (boost::iequals(logType, "none")) {
        m_service.GetLog().Info("Bars logging is disabled.");
      } else if (!boost::iequals(logType, "csv")) {
        m_service.GetLog().Error(
            "Wrong bars log type settings: \"%1%\". Unknown type."
            " Supported: none and CSV.",
            logType);
        throw Error("Wrong bars log type");
      } else {
        m_barsLog.reset(new std::ofstream);
      }
    }

    {
      const auto numberOfHistoryBars = configuration.ReadTypedKey<size_t>(
          "number_of_history_bars", m_numberOfHistoryBars);
      if (numberOfHistoryBars != m_numberOfHistoryBars) {
        if (m_countedBarSize) {
          throw Error(
              "Can't use \"number of history bars\""
              " with bar type \"number of updates\""
              " as history start time is unknown");
        }
        m_numberOfHistoryBars = numberOfHistoryBars;
      }
    }

    if (!m_countedBarSize) {
      m_isNewBar = boost::bind(&Implementation::IsNewTimedBar, this, _1, _2);
      m_onLevel1Tick =
          boost::bind(&Implementation::HandleLevel1Tick, this, _1, _2, _3);
      m_onNewBar =
          boost::bind(&Implementation::HandleNewTimedBar, this, _1, _2);

      m_service.GetLog().Info(
          "Stated with size \"%1%\" (number of history bars: %2%).",
          m_timedBarSize, m_numberOfHistoryBars);

    } else {
      m_isNewBar =
          boost::bind(&Implementation::IsNewTickCountedBar, this, _1, _2);
      m_onLevel1Tick = boost::bind(&Implementation::ThrowLevel1TickSupportError,
                                   this, _1, _2, _3);
      m_onNewBar =
          boost::bind(&Implementation::HandleNewCountedBar, this, _1, _2);

      m_service.GetLog().Info("Stated with size \"%1% %2%\".", m_countedBarSize,
                              m_unitsStr);
    }
  }

  void OpenLog(const pt::ptime &dataStartTime) {
    Assert(m_security);

    if (!m_barsLog || m_barsLog->is_open()) {
      return;
    }

    fs::path path = m_service.GetContext().GetSettings().GetLogsInstanceDir();
    path /= "Bars";
    if (!m_service.GetContext().GetSettings().IsReplayMode()) {
      boost::format fileName("%1%_%2%%3%_%4%_%5%_%6%");
      fileName % *m_security % m_unitsStr % m_barSizeStr %
          ConvertToFileName(dataStartTime) % m_service.GetId() %
          m_service.GetInstanceId();
      path /= SymbolToFileName(fileName.str(), "csv");
    } else {
      boost::format fileName("%1%_%2%%3%_%4%_%5%_%6%_%7%");
      fileName % *m_security % m_unitsStr % m_barSizeStr %
          ConvertToFileName(dataStartTime) %
          ConvertToFileName(m_service.GetContext().GetStartTime()) %
          m_service.GetId() % m_service.GetInstanceId();
      path /= SymbolToFileName(fileName.str(), "csv");
    }

    const bool isNew = !fs::exists(path);
    if (isNew) {
      fs::create_directories(path.branch_path());
    }
    m_barsLog->open(path.string(),
                    std::ios::out | std::ios::ate | std::ios::app);
    if (!*m_barsLog) {
      m_service.GetLog().Error("Failed to open log file %1%", path);
      throw Error("Failed to open log file");
    }
    if (isNew) {
      *m_barsLog << "Session" << csvDelimeter << "Date" << csvDelimeter
                 << "Time" << csvDelimeter << "Open" << csvDelimeter << "High"
                 << csvDelimeter << "Low" << csvDelimeter << "Close"
                 << csvDelimeter << "Volume";
      if (m_countedBarSize) {
        *m_barsLog << csvDelimeter << "Number of ticks";
      }
      *m_barsLog << std::endl;
    }
    *m_barsLog << std::setfill('0');

    m_service.GetLog().Info("Logging into %1%.", path);
  }

  void LogCurrentBar() const {
    Assert(m_current.bar);
    if (!m_barsLog || !m_current.bar) {
      return;
    }
    *m_barsLog << m_session;
    {
      const auto date = m_current.bar->time.date();
      *m_barsLog << csvDelimeter << date.year() << '.' << std::setw(2)
                 << date.month().as_number() << '.' << std::setw(2)
                 << date.day();
    }
    *m_barsLog << csvDelimeter
               << ExcelTextField(m_current.bar->time.time_of_day())
               << csvDelimeter
               << m_security->DescalePrice(m_current.bar->openTradePrice)
               << csvDelimeter
               << m_security->DescalePrice(m_current.bar->highTradePrice)
               << csvDelimeter
               << m_security->DescalePrice(m_current.bar->lowTradePrice)
               << csvDelimeter
               << m_security->DescalePrice(m_current.bar->closeTradePrice)
               << csvDelimeter << m_current.bar->tradingVolume;
    if (m_countedBarSize) {
      *m_barsLog << csvDelimeter << m_current.count;
    }
    *m_barsLog << std::endl;
  }

  void GetBarTimePoints(const pt::ptime &tradeTime,
                        pt::ptime &startTime,
                        pt::ptime &endTime) const {
    AssertNe(tradeTime, pt::not_a_date_time);
    AssertEq(startTime, pt::not_a_date_time);
    const auto time = tradeTime.time_of_day();
    static_assert(numberOfUnits == 6, "Units list changed.");
    switch (m_units) {
      case UNITS_SECONDS:
        endTime = pt::ptime(tradeTime.date()) + pt::hours(time.hours()) +
                  pt::minutes(time.minutes()) +
                  pt::seconds(((time.seconds() / m_barSizeUnits) + 1) *
                              m_barSizeUnits);
        startTime = endTime - m_timedBarSize;
        break;
      case UNITS_MINUTES:
        endTime = pt::ptime(tradeTime.date()) + pt::hours(time.hours()) +
                  pt::minutes(((time.minutes() / m_barSizeUnits) + 1) *
                              m_barSizeUnits);
        startTime = endTime - m_timedBarSize;
        break;
      case UNITS_HOURS:
        endTime =
            pt::ptime(tradeTime.date()) +
            pt::hours(((time.hours() / m_barSizeUnits) + 1) * m_barSizeUnits);
        startTime = endTime - m_timedBarSize;
        break;
      case UNITS_DAYS:
        //! @todo Implement days bar service
        throw Error("Days units is not implemented");
      case UNITS_WEEKS:
        //! @todo Implement days bar service
        throw Error("Weeks units is not implemented");
      case UNITS_TICKS:
        AssertEq(endTime, pt::not_a_date_time);
        startTime = tradeTime;
        break;
      default:
        AssertFail("Unknown units type");
        throw Error("Unknown bar service units type");
    }
  }

  template <typename Callback>
  bool ContinueBar(const Callback &callback) {
    Assert(!m_bars.empty());
    Assert(m_current.bar);
    callback(*m_current.bar);
    return false;
  }

  template <typename Callback>
  bool StartNewBar(const pt::ptime &time, const Callback &callback) {
    const bool isSignificantBar =
        m_current.bar && !IsZero(m_current.bar->lowTradePrice);
    if (isSignificantBar) {
      LogCurrentBar();
    }
    m_bars.resize(m_bars.size() + 1);
    m_current = Current(m_bars.back());
    GetBarTimePoints(time, m_current.bar->time, m_current.end);
    callback(*m_current.bar);
    return isSignificantBar;
  }

  bool CompleteBar() {
    if (!m_current.bar || IsZero(m_current.bar->lowTradePrice)) {
      return false;
    }
    LogCurrentBar();
    m_current = Current();
    return true;
  }

  template <typename Callback>
  bool AppendStat(const Security &security,
                  const pt::ptime &time,
                  const Callback &callback) {
    Assert(m_security == &security);
    UseUnused(security);

    bool isUpdated = false;

    if (!m_current.bar) {
      OpenLog(time);
      isUpdated = StartNewBar(time, callback);
    } else {
      isUpdated = m_isNewBar(time, true) ? StartNewBar(time, callback)
                                         : ContinueBar(callback);
    }

    if (m_isNewBar(time, false)) {
      isUpdated = CompleteBar() || isUpdated;
    }

    return isUpdated;
  }

  bool HandleNewTimedBar(const Security &security,
                         const Security::Bar &sourceBar) {
    if (!sourceBar.period) {
      m_service.GetLog().Error("Can't work with not timed source bar size.");
      throw MethodDoesNotSupportBySettings("Wrong source bar size");
    } else if (m_timedBarSize < *sourceBar.period) {
      m_service.GetLog().Error(
          "Can't work with source bar size %1%"
          " as service bar size is %2%.",
          *sourceBar.period, m_timedBarSize);
      throw MethodDoesNotSupportBySettings("Wrong source bar size");
    }

    const auto &setOpen = [](const Security::Bar &sourceBar,
                             ScaledPrice &stat) {
      if (sourceBar.openPrice && !stat) {
        stat = *sourceBar.openPrice;
      }
    };
    const auto &setClose = [](const Security::Bar &sourceBar,
                              ScaledPrice &stat) {
      if (sourceBar.closePrice) {
        stat = *sourceBar.closePrice;
      }
    };
    const auto &setMax = [](const Security::Bar &sourceBar, ScaledPrice &stat) {
      if (sourceBar.highPrice) {
        stat = std::max(stat, *sourceBar.highPrice);
      }
    };
    const auto &setMin = [](const Security::Bar &sourceBar, ScaledPrice &stat) {
      if (sourceBar.lowPrice) {
        stat = stat ? std::min(stat, *sourceBar.lowPrice) : *sourceBar.lowPrice;
      }
    };

    return AppendStat(security, sourceBar.time, [&](Bar &statBar) {
      static_assert(Security::Bar::numberOfTypes == 3,
                    "Bar type list changed.");
      switch (sourceBar.type) {
        case Security::Bar::TRADES:
          setOpen(sourceBar, statBar.openTradePrice);
          setClose(sourceBar, statBar.closeTradePrice);
          setMax(sourceBar, statBar.highTradePrice);
          setMin(sourceBar, statBar.lowTradePrice);
          break;
        case Security::Bar::BID:
          setMin(sourceBar, statBar.minBidPrice);
          setOpen(sourceBar, statBar.openBidPrice);
          setClose(sourceBar, statBar.closeBidPrice);
          break;
        case Security::Bar::ASK:
          setMax(sourceBar, statBar.maxAskPrice);
          setOpen(sourceBar, statBar.openAskPrice);
          setClose(sourceBar, statBar.closeAskPrice);
          break;
        default:
          AssertEq(Security::Bar::TRADES, sourceBar.type);
          return;
      }
    });
  }

  bool HandleNewCountedBar(const Security &security,
                           const Security::Bar &source) {
    if (source.period || !source.numberOfPoints) {
      m_service.GetLog().Error("Can't work with not counted source bar size.");
      throw MethodDoesNotSupportBySettings("Wrong source bar size");
    } else if (m_countedBarSize < *source.numberOfPoints) {
      m_service.GetLog().Error(
          "Can't work with source bar size %1%"
          " as service bar size is %2%.",
          *source.numberOfPoints, m_countedBarSize);
      throw MethodDoesNotSupportBySettings("Wrong source bar size");
    }

    return AppendStat(security, source.time, [this, &source](Bar &dest) {
      static_assert(Security::Bar::numberOfTypes == 3,
                    "Bar type list changed.");
      const auto count = m_current.count + *source.numberOfPoints;
      if (count > m_countedBarSize) {
        m_service.GetLog().Error(
            "New bar adds too many ticks %1%"
            ", but service bar size is %2%.",
            count, m_countedBarSize);
        throw MethodDoesNotSupportBySettings("Wrong source bar size");
      }
      m_current.count = count;
      switch (source.type) {
        case Security::Bar::TRADES:
          if (source.openPrice && !dest.openTradePrice) {
            dest.openTradePrice = *source.openPrice;
          }
          if (source.closePrice) {
            dest.closeTradePrice = *source.closePrice;
          }
          if (source.highPrice) {
            dest.highTradePrice =
                std::max(dest.highTradePrice, *source.highPrice);
          }
          if (source.lowPrice) {
            dest.lowTradePrice =
                dest.lowTradePrice
                    ? std::min(dest.lowTradePrice, *source.lowPrice)
                    : *source.lowPrice;
          }
          break;
        default:
          AssertEq(Security::Bar::TRADES, source.type);
          return;
      }
    });
  }

  bool HandleLevel1Tick(const Security &security,
                        const pt::ptime &time,
                        const Level1TickValue &value) {
    switch (value.GetType()) {
      case LEVEL1_TICK_BID_QTY:
      case LEVEL1_TICK_ASK_QTY:
      case LEVEL1_TICK_TRADING_VOLUME:
        return false;
    };

    return AppendStat(security, time, [this, &value](Bar &bar) {

      static_assert(numberOfLevel1TickTypes == 7, "List changed");

      switch (value.GetType()) {
        case LEVEL1_TICK_LAST_PRICE:
          bar.closeTradePrice = ScaledPrice(value.GetValue());
          if (!bar.openTradePrice) {
            AssertEq(0, bar.highTradePrice);
            AssertEq(0, bar.lowTradePrice);
            bar.openTradePrice = bar.highTradePrice = bar.lowTradePrice =
                bar.closeTradePrice;
          } else {
            AssertNe(0, bar.highTradePrice);
            AssertNe(0, bar.lowTradePrice);
            if (bar.highTradePrice < bar.closeTradePrice) {
              bar.highTradePrice = bar.closeTradePrice;
            }
            if (bar.lowTradePrice > bar.closeTradePrice) {
              bar.lowTradePrice = bar.closeTradePrice;
            }
            AssertLe(bar.lowTradePrice, bar.highTradePrice);
          }
          // Custom branch for Mrigesh Kejriwal uses NIFTY index which has no
          // bid and ask prices:
          /*
          RestoreBarFieldsFromPrevBar(
              bar.openBidPrice, bar.closeBidPrice, bar.minBidPrice,
              [](const Bar &bar) { return bar.closeBidPrice; },
              [this]() { return m_security->GetBidPriceScaled();
              });
          RestoreBarFieldsFromPrevBar(
              bar.openAskPrice, bar.closeAskPrice, bar.maxAskPrice,
              [](const Bar &bar) { return bar.closeAskPrice; },
              [this]() { return m_security->GetAskPriceScaled();
              });
          */
          break;

        case LEVEL1_TICK_LAST_QTY:
          bar.tradingVolume += value.GetValue();
          RestoreBarTradePriceFromPrevBar(bar);
          // Custom branch for Mrigesh Kejriwal uses NIFTY index which has no
          // bid and ask prices:
          /*
                  RestoreBarFieldsFromPrevBar(
              bar.openBidPrice, bar.closeBidPrice, bar.minBidPrice,
              [](const Bar &bar) { return bar.closeBidPrice; },
              [this]() { return m_security->GetBidPriceScaled(); });
          RestoreBarFieldsFromPrevBar(
              bar.openAskPrice, bar.closeAskPrice, bar.maxAskPrice,
              [](const Bar &bar) { return bar.closeAskPrice; },
              [this]() { return m_security->GetAskPriceScaled(); });
          */
          break;

        case LEVEL1_TICK_BID_PRICE:
          bar.closeBidPrice = ScaledPrice(value.GetValue());
          if (!bar.openBidPrice) {
            bar.openBidPrice = bar.closeBidPrice;
            AssertEq(0, bar.minBidPrice);
            bar.minBidPrice = bar.closeBidPrice;
          } else if (bar.minBidPrice > bar.closeBidPrice) {
            bar.minBidPrice = bar.closeBidPrice;
          }
          AssertNe(0, bar.openBidPrice);
          AssertNe(0, bar.minBidPrice);
          AssertNe(0, bar.closeBidPrice);
          RestoreBarTradePriceFromPrevBar(bar);
          // Custom branch for Mrigesh Kejriwal uses NIFTY index which has no
          // bid and ask prices:
          /*
          RestoreBarFieldsFromPrevBar(
              bar.openAskPrice, bar.closeAskPrice, bar.maxAskPrice,
              [](const Bar &bar) { return bar.closeAskPrice; },
              [this]() { return m_security->GetAskPriceScaled(); });
          */
          break;

        case LEVEL1_TICK_ASK_PRICE:
          bar.closeAskPrice = ScaledPrice(value.GetValue());
          if (!bar.openAskPrice) {
            bar.openAskPrice = bar.closeAskPrice;
            AssertEq(0, bar.maxAskPrice);
            bar.maxAskPrice = bar.closeAskPrice;
          } else if (bar.maxAskPrice < bar.closeAskPrice) {
            bar.maxAskPrice = bar.closeAskPrice;
          }
          AssertNe(0, bar.openAskPrice);
          AssertNe(0, bar.maxAskPrice);
          AssertNe(0, bar.closeAskPrice);
          RestoreBarTradePriceFromPrevBar(bar);
          // Custom branch for Mrigesh Kejriwal uses NIFTY index which has no
          // bid and ask prices:
          /*
          RestoreBarFieldsFromPrevBar(
              bar.openBidPrice, bar.closeBidPrice, bar.minBidPrice,
              [](const Bar &bar) { return bar.closeBidPrice; },
              [this]() { return m_security->GetBidPriceScaled(); });
          */
          break;

        default:
          AssertEq(LEVEL1_TICK_BID_PRICE, value.GetType());
          return;
      }

    });
  }

  bool ThrowLevel1TickSupportError(const Security &,
                                   const pt::ptime &,
                                   const Level1TickValue &) {
    throw MethodDoesNotSupportBySettings(
        "Bar service does not support work with level 1 ticks"
        " by the current settings (only timed bars supported)");
  }

  bool OnNewTrade(const Security &security,
                  const pt::ptime &time,
                  const ScaledPrice &price,
                  const Qty &qty) {
    return AppendStat(security, time, [&](Bar &bar) {
      if (IsZero(bar.openTradePrice)) {
        AssertEq(0, bar.highTradePrice);
        AssertEq(0, bar.lowTradePrice);
        AssertEq(0, bar.closeTradePrice);
        AssertEq(0, bar.tradingVolume);
        AssertNe(0, price);
        bar.openTradePrice = price;
      }
      bar.highTradePrice = std::max(bar.highTradePrice, price);
      bar.lowTradePrice =
          bar.lowTradePrice ? std::min(bar.lowTradePrice, price) : price;
      bar.closeTradePrice = price;
      bar.tradingVolume += qty;
      if (m_countedBarSize) {
        bar.time = time;
        ++m_current.count;
      }
    });
  }

  template <typename Stat>
  boost::shared_ptr<Stat> CreateStat(size_t size) const {
    return boost::shared_ptr<Stat>(new Stat(m_service, size));
  }

  void RestoreBarTradePriceFromPrevBar(Bar &bar) {
    if (bar.openTradePrice) {
      AssertLt(0, bar.closeTradePrice);
      AssertLt(0, bar.highTradePrice);
      AssertLt(0, bar.lowTradePrice);
      return;
    }
    if (m_bars.size() > 1) {
      bar.openTradePrice = (m_bars.rbegin() + 1)->closeTradePrice;
    } else {
      bar.openTradePrice = m_security->GetLastPriceScaled();
    }
    bar.highTradePrice = bar.lowTradePrice = bar.closeTradePrice =
        bar.openTradePrice;
  }

  template <typename GetBarClosePrice, typename GetCurrentPrice>
  void RestoreBarFieldsFromPrevBar(
      ScaledPrice &openPriceField,
      ScaledPrice &closePriceField,
      ScaledPrice &minMaxPriceField,
      const GetBarClosePrice &getBarClosePrice,
      const GetCurrentPrice &getCurrentPrice) const {
    if (openPriceField) {
      AssertLt(0, minMaxPriceField);
      AssertLt(0, closePriceField);
      return;
    }
    AssertEq(0, minMaxPriceField);
    AssertEq(0, closePriceField);
    if (m_bars.size() > 1) {
      openPriceField = getBarClosePrice(*(m_bars.rbegin() + 1));
    } else {
      openPriceField = getCurrentPrice();
    }
    minMaxPriceField = openPriceField;
    closePriceField = openPriceField;
  }

  bool IsNewTimedBar(const pt::ptime &time, bool isBefore) const {
    AssertNe(UNITS_TICKS, m_units);
    Assert(!m_current.end.is_not_a_date_time());
    AssertEq(0, m_countedBarSize);
    return isBefore ? m_current.end < time : m_current.end == time;
  }
  bool IsNewTickCountedBar(const pt::ptime &time, bool) const {
    AssertEq(UNITS_TICKS, m_units);
    Assert(m_current.end.is_not_a_date_time());
    AssertLt(0, m_countedBarSize);
    AssertLe(m_current.bar->time, time);
    UseUnused(time);
    return m_current.count >= m_countedBarSize;
  }
};

//////////////////////////////////////////////////////////////////////////

BarCollectionService::BarCollectionService(Context &context,
                                           const std::string &instanceName,
                                           const IniSectionRef &configuration)
    : Base(context,
           "{7619E946-BDE0-4FB6-94A6-CFFF3F183D92}",
           "Bars",
           instanceName,
           configuration),
      m_pimpl(new Implementation(*this, configuration)) {}

BarCollectionService::~BarCollectionService() {}

pt::ptime BarCollectionService::GetLastDataTime() const {
  return GetLastBar().time;
}

void BarCollectionService::OnSecurityStart(const Security &security,
                                           Security::Request &request) {
  if (m_pimpl->m_security) {
    throw Exception("Bar service works only with one security");
  }
  m_pimpl->m_security = &security;

  Base::OnSecurityStart(security, request);

  if (m_pimpl->m_countedBarSize) {
    request.RequestNumberOfTicks(m_pimpl->m_countedBarSize);
  } else if (!GetContext().GetSettings().IsReplayMode()) {
    auto time = GetContext().GetCurrentTime();
    time -= (m_pimpl->m_timedBarSize * int(m_pimpl->m_numberOfHistoryBars));
    request.RequestTime(time);
  }
}

bool BarCollectionService::OnNewBar(const Security &security,
                                    const Security::Bar &bar) {
  return m_pimpl->m_onNewBar(security, bar);
}

bool BarCollectionService::OnLevel1Tick(const Security &security,
                                        const pt::ptime &time,
                                        const Level1TickValue &value) {
  return m_pimpl->m_onLevel1Tick(security, time, value);
}

bool BarCollectionService::OnNewTrade(const Security &security,
                                      const pt::ptime &time,
                                      const ScaledPrice &price,
                                      const Qty &qty) {
  return m_pimpl->OnNewTrade(security, time, price, qty);
}

const Security &BarCollectionService::GetSecurity() const {
  if (!m_pimpl->m_security) {
    throw Error("Service does not have data security");
  }
  return *m_pimpl->m_security;
}

BarCollectionService::Bar BarCollectionService::GetBar(size_t index) const {
  if (index >= GetSize()) {
    throw BarDoesNotExistError(IsEmpty()
                                   ? "BarService is empty"
                                   : "Index is out of range of BarService");
  }
  AssertLt(index, m_pimpl->m_bars.size());
  return m_pimpl->m_bars[index];
}

BarCollectionService::Bar BarCollectionService::GetBarByReversedIndex(
    size_t index) const {
  if (index >= GetSize()) {
    throw BarDoesNotExistError(IsEmpty()
                                   ? "BarService is empty"
                                   : "Index is out of range of BarService");
  }
  AssertGt(m_pimpl->m_bars.size(), index);
  auto forwardIndex = m_pimpl->m_bars.size() - 1 - index;
  if (m_pimpl->m_current.bar) {
    Assert(m_pimpl->m_current.bar == &m_pimpl->m_bars.back());
    AssertGt(m_pimpl->m_bars.size() + 1, index);
    AssertLt(0, forwardIndex);
    --forwardIndex;
  }
  AssertLt(index, m_pimpl->m_bars.size());
  return m_pimpl->m_bars[forwardIndex];
}

BarCollectionService::Bar BarCollectionService::GetLastBar() const {
  return GetBarByReversedIndex(0);
}

size_t BarCollectionService::GetSize() const {
  auto result = m_pimpl->m_bars.size();
  if (m_pimpl->m_current.bar) {
    AssertLe(1, result);
    Assert(m_pimpl->m_current.bar == &m_pimpl->m_bars.back());
    result -= 1;
  }
  return result;
}

bool BarCollectionService::IsEmpty() const {
  return m_pimpl->m_bars.empty() ||
         (m_pimpl->m_bars.size() == 1 && m_pimpl->m_current.bar);
}

bool BarCollectionService::OnSecurityServiceEvent(
    const pt::ptime &time,
    const Security &security,
    const Security::ServiceEvent &securityEvent) {
  static_assert(Security::numberOfServiceEvents == 4, "List changed.");

  bool isUpdated = Base::OnSecurityServiceEvent(time, security, securityEvent);

  if (&security != m_pimpl->m_security) {
    return isUpdated;
  }

  switch (securityEvent) {
    case Security::SERVICE_EVENT_TRADING_SESSION_OPENED:
    case Security::SERVICE_EVENT_TRADING_SESSION_CLOSED:
      isUpdated = CompleteBar() || isUpdated;
      ++m_pimpl->m_session;
  }

  return isUpdated;
}

bool BarCollectionService::CompleteBar() { return m_pimpl->CompleteBar(); }

void BarCollectionService::ForEachReversed(
    const boost::function<bool(const Bar &)> &callback) const {
  auto it = m_pimpl->m_bars.crbegin();
  const auto end = m_pimpl->m_bars.crend();
  if (m_pimpl->m_current.bar) {
    Assert(&*it == m_pimpl->m_current.bar);
    Assert(it != end);
    ++it;
  }

  for (; it != end; ++it) {
    if (!callback(*it)) {
      return;
    }
  }
}

void BarCollectionService::DropLastBarCopy(
    const DropCopyDataSourceInstanceId &sourceId) const {
  if (IsEmpty()) {
    throw BarDoesNotExistError("BarService is empty");
  }

  GetContext().InvokeDropCopy([this, &sourceId](DropCopy &dropCopy) {
    const Bar &bar = GetLastBar();
    const Security &security = GetSecurity();
    dropCopy.CopyBar(sourceId, GetSize() - 1, bar.time,
                     security.DescalePrice(bar.openTradePrice),
                     security.DescalePrice(bar.highTradePrice),
                     security.DescalePrice(bar.lowTradePrice),
                     security.DescalePrice(bar.closeTradePrice));
  });
}

void BarCollectionService::DropUncompletedBarCopy(
    const DropCopyDataSourceInstanceId &sourceId) const {
  if (!m_pimpl->m_current.bar) {
    return;
  }

  GetContext().InvokeDropCopy([this, &sourceId](DropCopy &dropCopy) {
    const Security &security = GetSecurity();
    dropCopy.CopyBar(
        sourceId, m_pimpl->m_bars.size() - 1, m_pimpl->m_current.bar->time,
        security.DescalePrice(m_pimpl->m_current.bar->openTradePrice),
        security.DescalePrice(m_pimpl->m_current.bar->highTradePrice),
        security.DescalePrice(m_pimpl->m_current.bar->lowTradePrice),
        security.DescalePrice(m_pimpl->m_current.bar->closeTradePrice));
  });
}

void BarCollectionService::Reset() noexcept {
  m_pimpl->m_bars.clear();
  Assert(!m_pimpl->m_current.bar);
  AssertEq(0, m_pimpl->m_current.count);
  m_pimpl->m_current = Implementation::Current();
}

//////////////////////////////////////////////////////////////////////////

TRDK_SERVICES_API boost::shared_ptr<trdk::Service> CreateBarService(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<BarCollectionService>(context, instanceName,
                                                  configuration);
}

//////////////////////////////////////////////////////////////////////////
