/**************************************************************************
 *   Created: 2012/07/09 18:42:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Security.hpp"
#include "Context.hpp"
#include "DropCopy.hpp"
#include "MarketDataSource.hpp"
#include "Position.hpp"
#include "PriceBook.hpp"
#include "RiskControl.hpp"
#include "Settings.hpp"
#include "TradingLog.hpp"
#include "Common/ExpirationCalendar.hpp"

namespace fs = boost::filesystem;
namespace lt = boost::local_time;
namespace pt = boost::posix_time;
namespace sig = boost::signals2;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;

////////////////////////////////////////////////////////////////////////////////

namespace {

//! Level 1 data.
/** Should be native double to use NaN value as marker.
  */
typedef std::array<boost::atomic<double>, numberOfLevel1TickTypes> Level1;

bool IsSet(double value) { return !isnan(value); }
bool IsSet(const boost::atomic<double> &value) { return IsSet(value.load()); }

void Unset(boost::atomic<double> &val) noexcept {
  val = std::numeric_limits<double>::quiet_NaN();
}

std::string GetFutureSymbol(const Symbol &symbol) {
  AssertEq(SECURITY_TYPE_FUTURES, symbol.GetSecurityType());
  if (!symbol.IsExplicit()) {
    return symbol.GetSymbol();
  }
  boost::smatch match;
  const boost::regex expr("^([a-z]+)([a-z]\\d+|\\d{2,2}[a-z]{3,3}FUT)$",
                          boost::regex::icase);
  if (!boost::regex_match(symbol.GetSymbol(), match, expr)) {
    boost::format message("Failed to parse explicit future symbol \"%1%\"");
    message % symbol.GetSymbol();
    throw Exception(message.str().c_str());
  }
  return match[1];
}

//! Returns symbol price precision.
uint8_t GetPrecisionBySymbol(const Symbol &symbol) {
  switch (symbol.GetSecurityType()) {
    case SECURITY_TYPE_STOCK:
      if (symbol.GetSymbol() == "AAPL") {
        return 2;
      }
      break;
    case SECURITY_TYPE_FUTURES: {
      const auto &symbolStr = GetFutureSymbol(symbol);
      if (symbolStr == "CL") {
        return 2;
      } else if (symbolStr == "BR") {
        return 2;
      } else if (symbolStr == "GD") {
        return 1;
      } else if (symbolStr == "SV") {
        return 2;
      } else if (symbolStr == "SR") {
        return 0;
      } else if (symbolStr == "NIFTY" || symbolStr == "NIFTY50") {
        return 2;
      }
      break;
    }
    case SECURITY_TYPE_FOR:
      return 6;
    case SECURITY_TYPE_OPTIONS:
      if (symbol.GetSymbol() == "AAPL") {
        return 2;
      }
      break;
    case SECURITY_TYPE_INDEX:
      if (symbol.GetSymbol() == "NIFTY50") {
        return 2;
      } else if (symbol.GetSymbol() == "CL") {
        return 2;
      } else if (symbol.GetSymbol() == "SPX") {
        return 2;
      }
      break;
  }
  if (symbol.GetSymbol() == "TEST_SCALE2") {
    return 2;
  } else if (symbol.GetSymbol() == "TEST_SCALE4") {
    return 4;
  }
  boost::format message("Failed to find precision for unknown symbol \"%1%\"");
  message % symbol;
  throw Exception(message.str().c_str());
}

size_t GetQuoteSizeBySymbol(const Symbol &symbol) {
  switch (symbol.GetSecurityType()) {
    case SECURITY_TYPE_STOCK:
      if (symbol.GetSymbol() == "AAPL") {
        return 1;
      }
      break;
    case SECURITY_TYPE_FUTURES: {
      const auto &symbolStr = GetFutureSymbol(symbol);
      if (symbolStr == "CL") {
        return 1000;
      } else if (symbolStr == "BR") {
        return 10;
      } else if (symbolStr == "GD") {
        return 1;
      } else if (symbolStr == "SV") {
        return 10;
      } else if (symbolStr == "SR") {
        return 1;
      } else if (symbolStr == "NIFTY" || symbolStr == "NIFTY50") {
        return 1;
      }
      break;
    }
    case SECURITY_TYPE_FOR:
      return 1;
    case SECURITY_TYPE_OPTIONS:
      if (symbol.GetSymbol() == "AAPL") {
        return 1;
      }
      break;
    case SECURITY_TYPE_INDEX:
      if (symbol.GetSymbol() == "NIFTY50") {
        return 1;
      } else if (symbol.GetSymbol() == "CL") {
        return 1;
      } else if (symbol.GetSymbol() == "SPX") {
        return 1;
      }
      break;
  }
  if (symbol.GetSymbol() == "TEST_SCALE2" ||
      symbol.GetSymbol() == "TEST_SCALE4") {
    return 1;
  }
  boost::format message("Failed to find quote size for unknown symbol \"%1%\"");
  message % symbol;
  throw Exception(message.str().c_str());
}
}

////////////////////////////////////////////////////////////////////////////////

namespace {
namespace MarketDataLog {

class Record : public AsyncLogRecord {
 public:
  explicit Record(const pt::ptime &time, const Log::ThreadId &threadId)
      : AsyncLogRecord(time, threadId) {}

 public:
  const Record &operator>>(std::ostream &os) const {
    Dump(os, ",");
    return *this;
  }
};

std::ostream &operator<<(std::ostream &os, const Record &record) {
  record >> os;
  return os;
}

class OutStream : private boost::noncopyable {
 public:
  explicit OutStream(const lt::time_zone_ptr &timeZone) : m_log(timeZone) {}
  void Write(const Record &record) { m_log.Write(record); }
  bool IsEnabled() const { return m_log.IsEnabled(); }
  void EnableStream(std::ostream &os) { m_log.EnableStream(os, false); }
  pt::ptime GetTime() const { return m_log.GetTime(); }
  Log::ThreadId GetThreadId() const { return 0; }

 private:
  Log m_log;
};

typedef AsyncLog<Record, OutStream, TRDK_CONCURRENCY_PROFILE> LogBase;

class Log : private LogBase {
 public:
  typedef LogBase Base;

 public:
  using Base::IsEnabled;
  using Base::EnableStream;

  explicit Log(const Context &context, const Security &security)
      : Base(context.GetSettings().GetTimeZone()), m_security(security) {}

  template <typename... Params>
  void WriteLevel1Update(const pt::ptime &time, const Params &... params) {
    FormatAndWrite([&](Record &record) {
      record % record.GetTime() % time % "L1U";
      InsertFirstLevel1Update(record, params...);
    });
  }

  void WriteLevel1Update(const pt::ptime &time,
                         const std::vector<Level1TickValue> &ticks) {
    FormatAndWrite([&](Record &record) {
      record % record.GetTime() % time % "L1U";
      for (const auto &tick : ticks) {
        WriteValue(record, tick);
      }
    });
  }

  template <typename... Params>
  void WriteLevel1Tick(const pt::ptime &time, const Params &... params) {
    FormatAndWrite([&](Record &record) {
      record % record.GetTime() % time % "L1T";
      InsertFirstLevel1Update(record, params...);
    });
  }

  void WriteLevel1Tick(const pt::ptime &time,
                       const std::vector<Level1TickValue> &ticks) {
    FormatAndWrite([&](Record &record) {
      record % record.GetTime() % time % "L1T";
      for (const auto &tick : ticks) {
        WriteValue(record, tick);
      }
    });
  }

  void WriteTrade(const pt::ptime &time,
                  const Price &price,
                  const Qty &qty,
                  bool useAsLastTrade) {
    FormatAndWrite([&](Record &record) {
      record % record.GetTime() % time % "T" % price % qty % useAsLastTrade;
    });
  }

  void WriteBar(const Security::Bar &bar) {
    FormatAndWrite([this, &bar](Record &record) {
      record % record.GetTime() % bar.time % "B";
      static_assert(Security::Bar::numberOfTypes == 3, "List changed.");
      switch (bar.type) {
        default:
          AssertEq(Security::Bar::TRADES, bar.type);
          record % bar.type;
          break;
        case Security::Bar::TRADES:
          record % 'T';
          break;
        case Security::Bar::BID:
          record % 'B';
          break;
        case Security::Bar::ASK:
          record % 'A';
          break;
      }
      if (bar.openPrice) {
        record % *bar.openPrice;
      } else {
        record % '-';
      }
      if (bar.highPrice) {
        record % *bar.highPrice;
      } else {
        record % '-';
      }
      if (bar.lowPrice) {
        record % *bar.lowPrice;
      } else {
        record % '-';
      }
      if (bar.closePrice) {
        record % *bar.closePrice;
      } else {
        record % '-';
      }
      if (bar.volume) {
        record % *bar.volume;
      } else {
        record % '-';
      }
      if (bar.period) {
        record % *bar.period;
      } else {
        record % '-';
      }
      if (bar.numberOfPoints) {
        record % *bar.numberOfPoints;
      } else {
        record % '-';
      }
    });
  }

 private:
  template <typename... OtherParams>
  void InsertFirstLevel1Update(Record &record,
                               const Level1TickValue &tick,
                               const OtherParams &... otherParams) {
    WriteValue(record, tick);
    InsertFirstLevel1Update(record, otherParams...);
  }
  void InsertFirstLevel1Update(const Record &) {}

  void WriteValue(Record &record, const Level1TickValue &tick) {
    record % ConvertToPch(tick.GetType()) % tick.GetValue();
  }

 private:
  const Security &m_security;
};
}
}

////////////////////////////////////////////////////////////////////////////////

Security::Request::Request() : m_numberOfTicks(0) {}

Security::Request::operator bool() const {
  return m_numberOfTicks || !m_time.is_not_a_date_time();
}

void Security::Request::Swap(Request &rhs) throw() {
  std::swap(m_time, rhs.m_time);
  std::swap(m_numberOfTicks, rhs.m_numberOfTicks);
}

bool Security::Request::IsEarlier(const Request &rhs) const {
  if (!m_time.is_not_a_date_time()) {
    if (rhs.m_time.is_not_a_date_time()) {
      return true;
    } else if (m_time < rhs.m_time) {
      return true;
    }
  }

  return m_numberOfTicks > rhs.m_numberOfTicks;
}

void Security::Request::RequestNumberOfTicks(size_t numberOfTicks) {
  if (m_numberOfTicks > numberOfTicks) {
    return;
  }
  m_numberOfTicks = numberOfTicks;
}

void Security::Request::RequestTime(const pt::ptime &time) {
  Assert(!time.is_not_a_date_time());
  if (!m_time.is_not_a_date_time() && m_time < time) {
    return;
  }
  m_time = time;
}

void Security::Request::Merge(const Request &rhs) {
  if (!rhs.GetTime().is_not_a_date_time()) {
    RequestTime(rhs.GetTime());
  }
  RequestNumberOfTicks(rhs.GetNumberOfTicks());
}

size_t Security::Request::GetNumberOfTicks() const { return m_numberOfTicks; }

const pt::ptime &Security::Request::GetTime() const { return m_time; }

////////////////////////////////////////////////////////////////////////////////

Security::Bar::Bar(const pt::ptime &time, const Type &type)
    : time(time), type(type) {}

////////////////////////////////////////////////////////////////////////////////

Security::Exception::Exception(const char *what) : Lib::Exception(what) {}

Security::MarketDataValueDoesNotExist::MarketDataValueDoesNotExist(
    const char *what)
    : Exception(what) {}

////////////////////////////////////////////////////////////////////////////////

class Security::Implementation : private boost::noncopyable {
 public:
  template <typename SlotSignature>
  struct SignalTrait {
    typedef sig::signal<
        SlotSignature,
        sig::optional_last_value<
            typename boost::function_traits<SlotSignature>::result_type>,
        int,
        std::less<int>,
        boost::function<SlotSignature>,
        typename sig::detail::extended_signature<
            boost::function_traits<SlotSignature>::arity,
            SlotSignature>::function_type,
        sig::dummy_mutex>
        Signal;
  };

  Security &m_self;

  static boost::atomic<InstanceId> m_nextInstanceId;
  const InstanceId m_instanceId;

  MarketDataSource &m_source;

  boost::array<boost::shared_ptr<RiskControlSymbolContext>,
               numberOfTradingModes>
      m_riskControlContext;

  const uint8_t m_pricePrecision;
  const uintmax_t m_pricePrecisionPower;
  const size_t m_quoteSize;

  mutable SignalTrait<Level1UpdateSlotSignature>::Signal m_level1UpdateSignal;
  mutable SignalTrait<Level1TickSlotSignature>::Signal m_level1TickSignal;
  mutable SignalTrait<NewTradeSlotSignature>::Signal m_tradeSignal;
  mutable SignalTrait<BrokerPositionUpdateSlotSignature>::Signal
      m_brokerPositionUpdateSignal;
  mutable SignalTrait<NewBarSlotSignature>::Signal m_barSignal;
  mutable SignalTrait<BookUpdateTickSlotSignature>::Signal
      m_bookUpdateTickSignal;
  mutable SignalTrait<ServiceEventSlotSignature>::Signal m_serviceEventSignal;
  mutable SignalTrait<ContractSwitchingSlotSignature>::Signal
      m_contractSwitchedSignal;

  Level1 m_level1;
  boost::atomic_int64_t m_marketDataTime;
  boost::atomic_size_t m_numberOfMarketDataUpdates;
  mutable boost::atomic_bool m_isLevel1Started;
  const SupportedLevel1Types m_supportedLevel1Types;
  bool m_isOnline;
  bool m_isOpened;

  boost::optional<ContractExpiration> m_expiration;
  bool m_isContractSwitchingActive;

  Request m_request;

  std::ofstream m_marketDataLogFile;
  MarketDataLog::Log m_marketDataLog;

 public:
  Implementation(Security &self,
                 MarketDataSource &source,
                 const Symbol &symbol,
                 const SupportedLevel1Types &supportedLevel1Types)
      : m_self(self),
        m_instanceId(m_nextInstanceId++),
        m_source(source),
        m_pricePrecision(GetPrecisionBySymbol(symbol)),
        m_pricePrecisionPower(static_cast<decltype(m_pricePrecisionPower)>(
            std::pow(10, m_pricePrecision))),
        m_quoteSize(GetQuoteSizeBySymbol(symbol)),
        m_marketDataTime(0),
        m_numberOfMarketDataUpdates(0),
        m_isLevel1Started(false),
        m_supportedLevel1Types(supportedLevel1Types),
        m_isOnline(false),
        m_isOpened(false),
        m_isContractSwitchingActive(false),
        m_request({}),
        m_marketDataLog(m_source.GetContext(), self) {
    static_assert(numberOfTradingModes == 3, "List changed.");
    for (size_t i = 0; i < m_riskControlContext.size(); ++i) {
      m_riskControlContext[i] = m_source.GetContext()
                                    .GetRiskControl(static_cast<TradingMode>(i))
                                    .CreateSymbolContext(symbol);
    }

    for (auto &item : m_level1) {
      Unset(item);
    }

    if (m_source.GetContext().GetSettings().IsMarketDataLogEnabled()) {
      StartMarketDataLog(m_source.GetIndex());
    }
  }

  void CheckMarketDataUpdate(const pt::ptime &time) {
    if (time.is_not_a_date_time()) {
      return;
    }

#ifdef BOOST_ENABLE_ASSERT_HANDLER
    if (!GetLastMarketDataTime().is_not_a_date_time()) {
      AssertLe(GetLastMarketDataTime(), time);
    }
#endif

    m_marketDataTime = ConvertToMicroseconds(time);
    ++m_numberOfMarketDataUpdates;

    if (m_expiration && m_isOnline && !m_isContractSwitchingActive &&
        m_expiration->GetDate() <=
            time.date() +
                m_self.GetContext()
                    .GetSettings()
                    .GetPeriodBeforeExpiryDayToSwitchContract()) {
      m_isContractSwitchingActive = true;
      m_source.GetLog().Info(
          "Switching \"%1%\" to the next contract as last market data update"
          " is %2% and expiration date is %3%...",
          m_self, time, m_expiration->GetDate());
      m_source.SwitchToNextContract(m_self);
    }
    Assert(!m_expiration || m_expiration->GetDate() > time.date());
  }

  bool AddLevel1Tick(const pt::ptime &time,
                     const Level1TickValue &tick,
                     const Milestones &delayMeasurement,
                     bool flush,
                     bool isPreviouslyChanged) {
    const bool isChanged =
        SetLevel1(time, tick, delayMeasurement, flush, isPreviouslyChanged);
    CheckMarketDataUpdate(time);
    if (CheckLevel1Start()) {
      m_level1TickSignal(time, tick, delayMeasurement, flush);
    }
    return isChanged;
  }

  bool SetLevel1(const pt::ptime &time,
                 const Level1TickValue &tick,
                 const Milestones &delayMeasurement,
                 bool flush,
                 bool isPreviouslyChanged) {
    AssertLe(0, tick.GetValue());
    const bool isChanged =
        tick.GetValue() != m_level1[tick.GetType()].exchange(tick.GetValue()) ||
        isPreviouslyChanged;
    if (flush) {
      FlushLevel1Update(time, delayMeasurement, isChanged);
    }
    return isChanged;
  }

  void FlushLevel1Update(const pt::ptime &time,
                         const Milestones &delayMeasurement,
                         bool isChanged) {
    delayMeasurement.Measure(TimeMeasurement::SM_DISPATCHING_DATA_STORE);
    CheckMarketDataUpdate(time);
    if (isChanged && CheckLevel1Start()) {
      m_level1UpdateSignal(delayMeasurement);
    }
  }

  bool CheckLevel1Start() const {
    if (m_isLevel1Started) {
      return true;
    }
    for (size_t i = 0; i < m_level1.size(); ++i) {
      if (!IsSet(m_level1[i]) && m_supportedLevel1Types[i]) {
        return false;
      }
    }
    m_source.GetContext().GetLog().Info("%1% Level 1 started.", m_self);
    m_isLevel1Started = true;
    return true;
  }

  pt::ptime GetLastMarketDataTime() const {
    const auto marketDataTime = m_marketDataTime.load();
    return marketDataTime ? ConvertToPTimeFromMicroseconds(m_marketDataTime)
                          : pt::not_a_date_time;
  }

  template <Level1TickType tick>
  Double CheckAndGetLevel1Value(const Level1 &level1) const {
    const double value = level1[tick];
    if (!IsSet(value)) {
      Assert(IsSet(value));
      boost::format message(
          "Market data value \"%1%\" does not exist for \"%2%\"");
      message % ConvertToPch(tick) % m_self;
      throw MarketDataValueDoesNotExist(message.str().c_str());
    }
    AssertLe(0, value);
    return value;
  }

  template <Level1TickType tick>
  Double GetLevel1Value(const Level1 &level1) const {
    const double value = level1[tick];
    if (!IsSet(value)) {
      Assert(IsSet(value));
      boost::format message(
          "Market data value \"%1%\" does not exist for \"%2%\"");
      message % ConvertToPch(tick) % m_self;
      throw MarketDataValueDoesNotExist(message.str().c_str());
    }
    AssertLe(0, value);
    return value;
  }

  void StartMarketDataLog(size_t sourceIndex) {
    auto path = m_self.GetContext().GetSettings().GetLogsInstanceDir();
    path /= "MarketData";

    if (m_self.GetContext().GetSettings().IsReplayMode()) {
      throw Exception("Failed to start market data log file for replay mode");
    }
    boost::format fileName("%1%_%2%__%3%");
    fileName % m_self.GetSymbol()                                 // 1
        % sourceIndex                                             // 2
        % ConvertToFileName(m_self.GetContext().GetStartTime());  // 3
    path /= SymbolToFileName(fileName.str(), "csv");

    fs::create_directories(path.branch_path());
    m_marketDataLogFile.open(path.string(),
                             std::ios::out | std::ios::ate | std::ios::app);
    if (!m_marketDataLogFile.is_open()) {
      m_self.GetContext().GetLog().Error(
          "Failed to open market data log file %1%", path);
      throw Exception("Failed to open market data log file");
    }

    m_marketDataLog.EnableStream(m_marketDataLogFile);

    m_self.GetContext().GetLog().Info("Market data log for \"%1%\": %2%.",
                                      m_self, path);
  }
};

boost::atomic<Security::InstanceId> Security::Implementation::m_nextInstanceId(
    0);

//////////////////////////////////////////////////////////////////////////

Security::Security(Context &context,
                   const Symbol &symbol,
                   MarketDataSource &source,
                   const SupportedLevel1Types &supportedLevel1Types)
    : Base(context, symbol),
      m_pimpl(new Implementation(*this, source, symbol, supportedLevel1Types)) {
}

Security::~Security() = default;

const Security::InstanceId &Security::GetInstanceId() const {
  return m_pimpl->m_instanceId;
}

RiskControlSymbolContext &Security::GetRiskControlContext(
    const TradingMode &mode) {
  AssertLt(0, mode);
  AssertGe(m_pimpl->m_riskControlContext.size(), static_cast<size_t>(mode));
  // If context is not set - risk control is disabled and nobody should call
  // this method:
  Assert(m_pimpl->m_riskControlContext[mode - 1]);
  return *m_pimpl->m_riskControlContext[mode - 1];
}

const MarketDataSource &Security::GetSource() const {
  return m_pimpl->m_source;
}

size_t Security::GetQuoteSize() const { return m_pimpl->m_quoteSize; }

uintmax_t Security::GetPricePrecisionPower() const {
  return m_pimpl->m_pricePrecisionPower;
}

uint8_t Security::GetPricePrecision() const throw() {
  return m_pimpl->m_pricePrecision;
}

pt::ptime Security::GetLastMarketDataTime() const {
  return m_pimpl->GetLastMarketDataTime();
}

size_t Security::TakeNumberOfMarketDataUpdates() const {
  return m_pimpl->m_numberOfMarketDataUpdates.exchange(0);
}

bool Security::IsActive() const {
  return m_pimpl->m_isOnline && IsTradingSessionOpened() &&
         m_pimpl->m_isLevel1Started;
}

bool Security::IsOnline() const { return m_pimpl->m_isOnline; }

void Security::SetOnline(const pt::ptime &time, bool isOnline) {
  AssertNe(m_pimpl->m_isOnline, isOnline);
  if (m_pimpl->m_isOnline == isOnline) {
    return;
  }

  GetContext().GetLog().Info(
      "\"%1%\" now is %2% by the event %3%. Last data time: %4%.", *this,
      isOnline ? "online" : "offline", time, GetLastMarketDataTime());

  {
    const auto lock = GetSource().GetContext().SyncDispatching();
    m_pimpl->m_isOnline = isOnline;
    m_pimpl->m_serviceEventSignal(
        time, isOnline ? SERVICE_EVENT_ONLINE : SERVICE_EVENT_OFFLINE);
  }
}

bool Security::IsTradingSessionOpened() const { return m_pimpl->m_isOpened; }

void Security::SetTradingSessionState(const pt::ptime &time, bool isOpened) {
  AssertNe(m_pimpl->m_isOpened, isOpened);
  if (m_pimpl->m_isOpened == isOpened) {
    return;
  }

  GetContext().GetLog().Info(
      "\"%1%\" trading session is %2% by the event at %3%."
      " Last data time: %4%.",
      *this, isOpened ? "opened" : "closed", time, GetLastMarketDataTime());

  {
    const auto lock = GetSource().GetContext().SyncDispatching();
    m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
    m_pimpl->m_serviceEventSignal(time,
                                  isOpened
                                      ? SERVICE_EVENT_TRADING_SESSION_OPENED
                                      : SERVICE_EVENT_TRADING_SESSION_CLOSED);
  }
}

void Security::SwitchTradingSession(const pt::ptime &time) {
  GetContext().GetLog().Info(
      "%1% trading session is switched (%2%) by the event at %3%."
      " Last data time: %4%.",
      *this, m_pimpl->m_isOpened ? "opened" : "closed", time,
      GetLastMarketDataTime());

  {
    const auto lock = GetSource().GetContext().SyncDispatching();

    m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
    try {
      m_pimpl->m_serviceEventSignal(time,
                                    m_pimpl->m_isOpened
                                        ? SERVICE_EVENT_TRADING_SESSION_CLOSED
                                        : SERVICE_EVENT_TRADING_SESSION_OPENED);
    } catch (...) {
      m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
      throw;
    }

    m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
    m_pimpl->m_serviceEventSignal(time,
                                  m_pimpl->m_isOpened
                                      ? SERVICE_EVENT_TRADING_SESSION_OPENED
                                      : SERVICE_EVENT_TRADING_SESSION_CLOSED);
  }
}

void Security::SetRequest(const Request &request) {
  m_pimpl->m_request.Merge(request);
}

const Security::Request &Security::GetRequest() const {
  return m_pimpl->m_request;
}

Price Security::GetLastPrice() const {
  return m_pimpl->CheckAndGetLevel1Value<LEVEL1_TICK_LAST_PRICE>(
      m_pimpl->m_level1);
}

Qty Security::GetLastQty() const {
  return Qty(
      m_pimpl->CheckAndGetLevel1Value<LEVEL1_TICK_LAST_QTY>(m_pimpl->m_level1));
}

Qty Security::GetTradedVolume() const {
  return Qty(m_pimpl->CheckAndGetLevel1Value<LEVEL1_TICK_TRADING_VOLUME>(
      m_pimpl->m_level1));
}

Price Security::GetAskPrice() const {
  return m_pimpl->CheckAndGetLevel1Value<LEVEL1_TICK_ASK_PRICE>(
      m_pimpl->m_level1);
}
Price Security::GetAskPriceValue() const {
  return m_pimpl->m_level1[LEVEL1_TICK_ASK_PRICE];
}

Qty Security::GetAskQty() const {
  return Qty(
      m_pimpl->CheckAndGetLevel1Value<LEVEL1_TICK_ASK_QTY>(m_pimpl->m_level1));
}
Qty Security::GetAskQtyValue() const {
  return m_pimpl->m_level1[LEVEL1_TICK_ASK_QTY];
}

Price Security::GetBidPrice() const {
  return m_pimpl->GetLevel1Value<LEVEL1_TICK_BID_PRICE>(m_pimpl->m_level1);
}
Price Security::GetBidPriceValue() const {
  return m_pimpl->m_level1[LEVEL1_TICK_BID_PRICE];
}

Qty Security::GetBidQty() const {
  return Qty(
      m_pimpl->CheckAndGetLevel1Value<LEVEL1_TICK_BID_QTY>(m_pimpl->m_level1));
}
Qty Security::GetBidQtyValue() const {
  return m_pimpl->m_level1[LEVEL1_TICK_BID_QTY];
}

Security::ContractSwitchingSlotConnection
Security::SubscribeToContractSwitching(
    const ContractSwitchingSlot &slot) const {
  return m_pimpl->m_contractSwitchedSignal.connect(slot);
}

Security::Level1UpdateSlotConnection Security::SubscribeToLevel1Updates(
    const Level1UpdateSlot &slot) const {
  return m_pimpl->m_level1UpdateSignal.connect(slot);
}

Security::Level1UpdateSlotConnection Security::SubscribeToLevel1Ticks(
    const Level1TickSlot &slot) const {
  return m_pimpl->m_level1TickSignal.connect(slot);
}

Security::NewTradeSlotConnection Security::SubscribeToTrades(
    const NewTradeSlot &slot) const {
  return m_pimpl->m_tradeSignal.connect(slot);
}

Security::NewTradeSlotConnection Security::SubscribeToBrokerPositionUpdates(
    const BrokerPositionUpdateSlot &slot) const {
  return m_pimpl->m_brokerPositionUpdateSignal.connect(slot);
}

Security::NewBarSlotConnection Security::SubscribeToBars(
    const NewBarSlot &slot) const {
  return m_pimpl->m_barSignal.connect(slot);
}

Security::BookUpdateTickSlotConnection Security::SubscribeToBookUpdateTicks(
    const BookUpdateTickSlot &slot) const {
  return m_pimpl->m_bookUpdateTickSignal.connect(slot);
}

Security::ServiceEventSlotConnection Security::SubscribeToServiceEvents(
    const ServiceEventSlot &slot) const {
  return m_pimpl->m_serviceEventSignal.connect(slot);
}

bool Security::IsLevel1Required() const {
  return IsLevel1UpdatesRequired() || IsLevel1TicksRequired();
}

bool Security::IsLevel1UpdatesRequired() const {
  return !m_pimpl->m_level1UpdateSignal.empty();
}

bool Security::IsLevel1TicksRequired() const {
  return !m_pimpl->m_level1TickSignal.empty();
}

bool Security::IsTradesRequired() const {
  return !m_pimpl->m_tradeSignal.empty();
}

bool Security::IsBrokerPositionRequired() const {
  return !m_pimpl->m_brokerPositionUpdateSignal.empty();
}

bool Security::IsBarsRequired() const { return !m_pimpl->m_barSignal.empty(); }

void Security::SetLevel1(const pt::ptime &time,
                         const Level1TickValue &tick,
                         const Milestones &delayMeasurement) {
  SetLevel1(time, tick, true, false, delayMeasurement);
}

bool Security::SetLevel1(const pt::ptime &time,
                         const Level1TickValue &tick,
                         bool flush,
                         bool isPreviouslyChanged,
                         const Milestones &delayMeasurement) {
  if (!m_pimpl->SetLevel1(time, tick, delayMeasurement, flush,
                          isPreviouslyChanged)) {
    return false;
  }
  GetContext().InvokeDropCopy([this, &time, &tick](DropCopy &dropCopy) {
    dropCopy.CopyLevel1(*this, time, tick);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick);
  return true;
}

void Security::SetLevel1(const pt::ptime &time,
                         const Level1TickValue &tick1,
                         const Level1TickValue &tick2,
                         const Milestones &delayMeasurement) {
  AssertNe(tick1.GetType(), tick2.GetType());
  if (!m_pimpl->SetLevel1(
          time, tick2, delayMeasurement, true,
          m_pimpl->SetLevel1(time, tick1, delayMeasurement, false, false))) {
    return;
  }
  GetContext().InvokeDropCopy([this, &time, &tick1, &tick2](
      DropCopy &dropCopy) { dropCopy.CopyLevel1(*this, time, tick1, tick2); });
  m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick1, tick2);
}

void Security::SetLevel1(const pt::ptime &time,
                         const Level1TickValue &tick1,
                         const Level1TickValue &tick2,
                         const Level1TickValue &tick3,
                         const Milestones &delayMeasurement) {
  AssertNe(tick1.GetType(), tick2.GetType());
  AssertNe(tick1.GetType(), tick3.GetType());
  AssertNe(tick2.GetType(), tick3.GetType());
  if (!m_pimpl->SetLevel1(
          time, tick3, delayMeasurement, true,
          m_pimpl->SetLevel1(time, tick2, delayMeasurement, false,
                             m_pimpl->SetLevel1(time, tick1, delayMeasurement,
                                                false, false)))) {
    return;
  }
  GetContext().InvokeDropCopy(
      [this, &time, &tick1, &tick2, &tick3](DropCopy &dropCopy) {
        dropCopy.CopyLevel1(*this, time, tick1, tick2, tick3);
      });
  m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick1, tick2, tick3);
}

void Security::SetLevel1(const pt::ptime &time,
                         const Level1TickValue &tick1,
                         const Level1TickValue &tick2,
                         const Level1TickValue &tick3,
                         const Level1TickValue &tick4,
                         const Milestones &delayMeasurement) {
  AssertNe(tick1.GetType(), tick2.GetType());
  AssertNe(tick1.GetType(), tick3.GetType());
  AssertNe(tick1.GetType(), tick4.GetType());
  AssertNe(tick2.GetType(), tick4.GetType());
  AssertNe(tick3.GetType(), tick2.GetType());
  AssertNe(tick3.GetType(), tick4.GetType());
  if (!m_pimpl->SetLevel1(
          time, tick4, delayMeasurement, true,
          m_pimpl->SetLevel1(
              time, tick3, delayMeasurement, false,
              m_pimpl->SetLevel1(
                  time, tick2, delayMeasurement, false,
                  m_pimpl->SetLevel1(time, tick1, delayMeasurement, false,
                                     false))))) {
    return;
  }
  GetContext().InvokeDropCopy(
      [this, &time, &tick1, &tick2, &tick3, &tick4](DropCopy &dropCopy) {
        dropCopy.CopyLevel1(*this, time, tick1, tick2, tick3, tick4);
      });
  m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick1, tick2, tick3, tick4);
}

void Security::SetLevel1(const pt::ptime &time,
                         const std::vector<Level1TickValue> &ticks,
                         const Milestones &delayMeasurement) {
  size_t counter = 0;
  bool isPreviousChanged = false;
  for (const auto &tick : ticks) {
    isPreviousChanged =
        m_pimpl->SetLevel1(time, tick, delayMeasurement,
                           ++counter >= ticks.size(), isPreviousChanged);
  }
  if (!isPreviousChanged) {
    return;
  }
  GetContext().InvokeDropCopy([this, &time, &ticks](DropCopy &dropCopy) {
    dropCopy.CopyLevel1(*this, time, ticks);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Update(time, ticks);
}

bool Security::AddLevel1Tick(const pt::ptime &time,
                             const Level1TickValue &tick,
                             bool flush,
                             bool isPreviouslyChanged,
                             const Milestones &delayMeasurement) {
  const bool result = m_pimpl->AddLevel1Tick(time, tick, delayMeasurement,
                                             flush, isPreviouslyChanged);
  GetContext().InvokeDropCopy([this, &time, &tick](DropCopy &dropCopy) {
    dropCopy.CopyLevel1(*this, time, tick);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, tick);
  return result;
}

void Security::AddLevel1Tick(const pt::ptime &time,
                             const Level1TickValue &tick,
                             const Milestones &delayMeasurement) {
  m_pimpl->AddLevel1Tick(time, tick, delayMeasurement, true, false);
  GetContext().InvokeDropCopy(
      [&](DropCopy &dropCopy) { dropCopy.CopyLevel1(*this, time, tick); });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, tick);
}

void Security::AddLevel1Tick(const pt::ptime &time,
                             const Level1TickValue &tick1,
                             const Level1TickValue &tick2,
                             const Milestones &delayMeasurement) {
  m_pimpl->AddLevel1Tick(
      time, tick2, delayMeasurement, true,
      m_pimpl->AddLevel1Tick(time, tick1, delayMeasurement, false, false));
  GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {
    dropCopy.CopyLevel1(*this, time, tick1, tick2);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, tick1, tick2);
}

void Security::AddLevel1Tick(const pt::ptime &time,
                             const Level1TickValue &tick1,
                             const Level1TickValue &tick2,
                             const Level1TickValue &tick3,
                             const Milestones &delayMeasurement) {
  m_pimpl->AddLevel1Tick(
      time, tick3, delayMeasurement, true,
      m_pimpl->AddLevel1Tick(
          time, tick2, delayMeasurement, false,
          m_pimpl->AddLevel1Tick(time, tick1, delayMeasurement, false, false)));
  GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {
    dropCopy.CopyLevel1(*this, time, tick1, tick2, tick3);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, tick1, tick2, tick3);
}

void Security::AddLevel1Tick(const pt::ptime &time,
                             const Level1TickValue &tick1,
                             const Level1TickValue &tick2,
                             const Level1TickValue &tick3,
                             const Level1TickValue &tick4,
                             const Milestones &delayMeasurement) {
  m_pimpl->AddLevel1Tick(
      time, tick4, delayMeasurement, true,
      m_pimpl->AddLevel1Tick(
          time, tick3, delayMeasurement, false,
          m_pimpl->AddLevel1Tick(
              time, tick2, delayMeasurement, false,
              m_pimpl->AddLevel1Tick(time, tick1, delayMeasurement, false,
                                     false))));
  GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {
    dropCopy.CopyLevel1(*this, time, tick1, tick2, tick3, tick4);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, tick1, tick2, tick4);
}

void Security::AddLevel1Tick(const pt::ptime &time,
                             const std::vector<trdk::Level1TickValue> &ticks,
                             const Milestones &delayMeasurement) {
  size_t counter = 0;
  bool isPreviousChanged = false;
  for (const auto &tick : ticks) {
    isPreviousChanged =
        m_pimpl->AddLevel1Tick(time, tick, delayMeasurement,
                               ++counter >= ticks.size(), isPreviousChanged);
  }
  GetContext().InvokeDropCopy(
      [&](DropCopy &dropCopy) { dropCopy.CopyLevel1(*this, time, ticks); });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, ticks);
}

void Security::AddTrade(const pt::ptime &time,
                        const Price &price,
                        const Qty &qty,
                        const Milestones &delayMeasurement,
                        bool useAsLastTrade) {
  AssertLt(0, price);
  AssertLt(0, qty);

  m_pimpl->CheckMarketDataUpdate(time);
  m_pimpl->m_tradeSignal(time, price, qty, delayMeasurement);

  if (useAsLastTrade) {
    m_pimpl->SetLevel1(
        time, Level1TickValue::Create<LEVEL1_TICK_LAST_QTY>(qty),
        delayMeasurement, true,
        m_pimpl->SetLevel1(
            time, Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(price),
            delayMeasurement, false, false));
  }

  m_pimpl->m_marketDataLog.WriteTrade(time, price, qty, useAsLastTrade);
}

void Security::AddBar(Bar &&bar) {
  m_pimpl->CheckMarketDataUpdate(bar.time);
  m_pimpl->m_barSignal(bar);
  m_pimpl->m_marketDataLog.WriteBar(bar);
}

void Security::SetBrokerPosition(bool isLong,
                                 const Qty &qty,
                                 const Volume &volume,
                                 bool isInitial) {
  m_pimpl->m_brokerPositionUpdateSignal(isLong, qty, volume, isInitial);
}

void Security::SetBook(PriceBook &book, const Milestones &delayMeasurement) {
  AssertNe(pt::not_a_date_time, book.GetTime());

#if defined(BOOST_ENABLE_ASSERT_HANDLER)
  {
    for (size_t i = 0; i < book.GetBid().GetSize(); ++i) {
      Assert(book.GetBid().GetLevel(i).GetPrice() != 0);
      Assert(book.GetBid().GetLevel(i).GetQty() != 0);
      AssertNe(pt::not_a_date_time, book.GetBid().GetLevel(i).GetTime());
      if (i > 0) {
        AssertGt(book.GetBid().GetLevel(i - 1).GetPrice(),
                 book.GetBid().GetLevel(i).GetPrice());
      }
    }
    for (size_t i = 0; i < book.GetAsk().GetSize(); ++i) {
      Assert(book.GetAsk().GetLevel(i).GetPrice() != 0);
      AssertNe(0, book.GetAsk().GetLevel(i).GetQty());
      AssertNe(pt::not_a_date_time, book.GetAsk().GetLevel(i).GetTime());
      if (i > 0) {
        AssertLt(book.GetAsk().GetLevel(i - 1).GetPrice(),
                 book.GetAsk().GetLevel(i).GetPrice());
      }
    }
  }
#endif

  GetContext().InvokeDropCopy(
      [this, &book](DropCopy &dropCopy) { dropCopy.CopyBook(*this, book); });

  // Adjusting:
  while (!book.GetBid().IsEmpty() && !book.GetAsk().IsEmpty() &&
         book.GetBid().GetTop().GetPrice() >
             book.GetAsk().GetTop().GetPrice()) {
    bool isBidOlder =
        book.GetBid().GetTop().GetTime() < book.GetAsk().GetTop().GetTime();
    if (isBidOlder) {
      book.GetBid().PopTop();
    } else {
      book.GetAsk().PopTop();
    }
  }

  SetLevel1(
      book.GetTime(),
      Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
          book.GetBid().IsEmpty() ? 0 : book.GetBid().GetTop().GetPrice()),
      Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(
          book.GetBid().IsEmpty() ? Qty(0) : book.GetBid().GetTop().GetQty()),
      Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
          book.GetAsk().IsEmpty() ? 0 : book.GetAsk().GetTop().GetPrice()),
      Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(
          book.GetAsk().IsEmpty() ? Qty(0) : book.GetAsk().GetTop().GetQty()),
      delayMeasurement);
  m_pimpl->CheckMarketDataUpdate(book.GetTime());

  m_pimpl->m_bookUpdateTickSignal(book, delayMeasurement);
}

const ContractExpiration &Security::GetExpiration() const {
  if (!m_pimpl->m_expiration) {
    boost::format error("%1% doesn't have expiration");
    error % *this;
    throw LogicError(error.str().c_str());
  }
  return *m_pimpl->m_expiration;
}

bool Security::HasExpiration() const {
  return m_pimpl->m_expiration ? true : false;
}

bool Security::SetExpiration(const pt::ptime &time,
                             const ContractExpiration &expiration) {
  return SetExpiration(time,
                       std::forward<const ContractExpiration>(expiration));
}

bool Security::SetExpiration(const pt::ptime &time,
                             const ContractExpiration &&expiration) {
#ifdef BOOST_ENABLE_ASSERT_HANDLER
  if (m_pimpl->m_expiration) {
    AssertLt(*m_pimpl->m_expiration, expiration);
  }
#endif

  const bool isFirstContract = m_pimpl->m_expiration ? false : true;

  GetSource().GetLog().Info(
      m_pimpl->m_expiration
          ? "Switching \"%1%\" to the next contract %2% by the event at %3%..."
          : "Starting \"%1%\" with the contract %2% by the event at %3%...",
      *this, expiration.GetDate(), time);

  {
    const auto lock = GetSource().GetContext().SyncDispatching();

    Assert(m_pimpl->m_isContractSwitchingActive || !m_pimpl->m_expiration);

    if (m_pimpl->m_expiration) {
      m_pimpl->m_request = {};
    }

    Request request;
    bool isSwitched = true;
    m_pimpl->m_contractSwitchedSignal(time, request, isSwitched);
    AssertEq(0, m_pimpl->m_request.GetNumberOfTicks());
    Assert(m_pimpl->m_request.GetTime().is_not_a_date_time());
    if (!isSwitched) {
      m_pimpl->m_isContractSwitchingActive = true;
      GetSource().GetLog().Info(
          "Switching \"%1%\" to the next contract is delayed.", *this);
      return false;
    }

    m_pimpl->m_isContractSwitchingActive = false;
    m_pimpl->m_isLevel1Started = false;
    m_pimpl->m_expiration = std::move(expiration);
    for (auto &item : m_pimpl->m_level1) {
      Unset(item);
    }
    m_pimpl->m_marketDataTime = 0;

    if (request.IsEarlier(m_pimpl->m_request)) {
      m_pimpl->m_request.Merge(request);
      return true;
    } else {
      return false;
    }
  }
}

void Security::ContinueContractSwitchingToNextExpiration() {
  Assert(m_pimpl->m_isContractSwitchingActive);
  GetSource().SwitchToNextContract(*this);
}

////////////////////////////////////////////////////////////////////////////////
