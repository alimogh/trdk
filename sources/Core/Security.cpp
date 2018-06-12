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
#include "Bar.hpp"
#include "Context.hpp"
#include "DropCopy.hpp"
#include "MarketDataSource.hpp"
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
using namespace Lib;
using namespace TimeMeasurement;

////////////////////////////////////////////////////////////////////////////////

namespace {

typedef boost::shared_mutex StartedBarsMutex;
typedef boost::shared_lock<StartedBarsMutex> StartedBarsReadLock;
typedef boost::unique_lock<StartedBarsMutex> StartedBarsWriteLock;
struct StartedBarsHash {
  std::size_t operator()(const pt::time_duration& value) const {
    return boost::hash_value(value.total_nanoseconds());
  }
};
typedef boost::unordered_map<pt::time_duration,
                             std::pair<pt::ptime, size_t>,
                             StartedBarsHash>
    StartedBars;

//! Level 1 data.
/** Should be native double to use NaN value as marker.
 */
typedef std::array<boost::atomic<double>, numberOfLevel1TickTypes> Level1;

bool IsSet(const double value) { return !isnan(value); }
bool IsSet(const boost::atomic<double>& value) { return IsSet(value.load()); }

void Unset(boost::atomic<double>& val) noexcept {
  val = std::numeric_limits<double>::quiet_NaN();
}

std::string GetFutureSymbol(const Symbol& symbol) {
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
uint8_t GetPrecisionBySymbol(const Symbol& symbol) {
  switch (symbol.GetSecurityType()) {
    case SECURITY_TYPE_STOCK:
      if (symbol.GetSymbol() == "AAPL") {
        return 2;
      }
      break;
    case SECURITY_TYPE_FUTURES: {
      const auto& symbolStr = GetFutureSymbol(symbol);
      if (symbolStr == "CL") {
        return 2;
      }
      if (symbolStr == "BR") {
        return 2;
      }
      if (symbolStr == "GD") {
        return 1;
      }
      if (symbolStr == "SV") {
        return 2;
      }
      if (symbolStr == "SR") {
        return 0;
      }
      if (symbolStr == "NIFTY" || symbolStr == "NIFTY50") {
        return 2;
      }
      break;
    }
    case SECURITY_TYPE_FOR: {
      if (boost::iends_with(symbol.GetSymbol(), "JPY")) {
        return 3;
      }
      return 5;
    }
    case SECURITY_TYPE_OPTIONS:
      if (symbol.GetSymbol() == "AAPL") {
        return 2;
      }
      break;
    case SECURITY_TYPE_INDEX: {
      if (symbol.GetSymbol() == "NIFTY50") {
        return 2;
      }
      if (symbol.GetSymbol() == "CL") {
        return 2;
      }
      if (symbol.GetSymbol() == "SPX") {
        return 2;
      }
    } break;
    case SECURITY_TYPE_CRYPTO:
      return 8;
    default:
      break;
  }
  if (symbol.GetSymbol() == "TEST_SCALE2") {
    return 2;
  }
  if (symbol.GetSymbol() == "TEST_SCALE4") {
    return 4;
  }
  boost::format message("Failed to find precision for unknown symbol \"%1%\"");
  message % symbol;
  throw Exception(message.str().c_str());
}

size_t GetNumberOfItemsPerQtyBySymbol(const Symbol& symbol) {
  switch (symbol.GetSecurityType()) {
    case SECURITY_TYPE_STOCK:
      if (symbol.GetSymbol() == "AAPL") {
        return 1;
      }
      break;
    case SECURITY_TYPE_FUTURES: {
      const auto& symbolStr = GetFutureSymbol(symbol);
      if (symbolStr == "CL") {
        return 1000;
      }
      if (symbolStr == "BR") {
        return 10;
      }
      if (symbolStr == "GD") {
        return 1;
      }
      if (symbolStr == "SV") {
        return 10;
      }
      if (symbolStr == "SR") {
        return 1;
      }
      if (symbolStr == "NIFTY" || symbolStr == "NIFTY50") {
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
    case SECURITY_TYPE_INDEX: {
      if (symbol.GetSymbol() == "NIFTY50") {
        return 1;
      }
      if (symbol.GetSymbol() == "CL") {
        return 1;
      }
      if (symbol.GetSymbol() == "SPX") {
        return 1;
      }
    } break;
    case SECURITY_TYPE_CRYPTO:
      return 1;
    default:
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

Qty GetLotSizeBySymbol(const Symbol& symbol) {
  switch (symbol.GetSecurityType()) {
    case SECURITY_TYPE_FOR:
      return 100000;
    default:
      return 1;
  }
}
}  // namespace

////////////////////////////////////////////////////////////////////////////////

namespace {
namespace MarketDataLog {

class Record : public AsyncLogRecord {
 public:
  explicit Record(const pt::ptime& time, const Log::ThreadId& threadId)
      : AsyncLogRecord(time, threadId) {}

 public:
  const Record& operator>>(std::ostream& os) const {
    Dump(os, ",");
    return *this;
  }
};

std::ostream& operator<<(std::ostream& os, const Record& record) {
  record >> os;
  return os;
}

class OutStream : private boost::noncopyable {
 public:
  explicit OutStream(const lt::time_zone_ptr& timeZone) : m_log(timeZone) {}
  void Write(const Record& record) { m_log.Write(record); }
  bool IsEnabled() const { return m_log.IsEnabled(); }
  void EnableStream(std::ostream& os) { m_log.EnableStream(os, false); }
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
  using Base::EnableStream;
  using Base::IsEnabled;

  explicit Log(const Context& context)
      : Base(context.GetSettings().GetTimeZone()) {}

  template <typename... Params>
  void WriteLevel1Update(const pt::ptime& time, const Params&... params) {
    FormatAndWrite([&](Record& record) {
      record % record.GetTime() % time % "L1U";
      InsertFirstLevel1Update(record, params...);
    });
  }

  void WriteLevel1Update(const pt::ptime& time,
                         const std::vector<Level1TickValue>& ticks) {
    FormatAndWrite([&](Record& record) {
      record % record.GetTime() % time % "L1U";
      for (const auto& tick : ticks) {
        WriteValue(record, tick);
      }
    });
  }

  template <typename... Params>
  void WriteLevel1Tick(const pt::ptime& time, const Params&... params) {
    FormatAndWrite([&](Record& record) {
      record % record.GetTime() % time % "L1T";
      InsertFirstLevel1Update(record, params...);
    });
  }

  void WriteLevel1Tick(const pt::ptime& time,
                       const std::vector<Level1TickValue>& ticks) {
    FormatAndWrite([&](Record& record) {
      record % record.GetTime() % time % "L1T";
      for (const auto& tick : ticks) {
        WriteValue(record, tick);
      }
    });
  }

  void WriteTrade(const pt::ptime& time,
                  const Price& price,
                  const Qty& qty,
                  bool useAsLastTrade) {
    FormatAndWrite([&](Record& record) {
      record % record.GetTime() % time % "T" % price % qty % useAsLastTrade;
    });
  }

 private:
  template <typename... OtherParams>
  void InsertFirstLevel1Update(Record& record,
                               const Level1TickValue& tick,
                               const OtherParams&... otherParams) {
    WriteValue(record, tick);
    InsertFirstLevel1Update(record, otherParams...);
  }

  static void InsertFirstLevel1Update(const Record&) {}

  void WriteValue(Record& record, const Level1TickValue& tick) {
    record % ConvertToPch(tick.GetType()) % tick.GetValue();
  }
};
}  // namespace MarketDataLog
}  // namespace

////////////////////////////////////////////////////////////////////////////////

Security::Request::Request() : m_numberOfTicks(0) {}

Security::Request::operator bool() const {
  return m_numberOfTicks || !m_time.is_not_a_date_time();
}

void Security::Request::Swap(Request& rhs) noexcept {
  std::swap(m_time, rhs.m_time);
  std::swap(m_numberOfTicks, rhs.m_numberOfTicks);
}

bool Security::Request::IsEarlier(const Request& rhs) const {
  if (!m_time.is_not_a_date_time()) {
    if (rhs.m_time.is_not_a_date_time()) {
      return true;
    }
    if (m_time < rhs.m_time) {
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

void Security::Request::RequestTime(const pt::ptime& time) {
  Assert(!time.is_not_a_date_time());
  if (!m_time.is_not_a_date_time() && m_time < time) {
    return;
  }
  m_time = time;
}

void Security::Request::Merge(const Request& rhs) {
  if (!rhs.GetTime().is_not_a_date_time()) {
    RequestTime(rhs.GetTime());
  }
  RequestNumberOfTicks(rhs.GetNumberOfTicks());
}

size_t Security::Request::GetNumberOfTicks() const { return m_numberOfTicks; }

const pt::ptime& Security::Request::GetTime() const { return m_time; }

////////////////////////////////////////////////////////////////////////////////

Security::Exception::Exception(const char* what) : Lib::Exception(what) {}

Security::MarketDataValueDoesNotExist::MarketDataValueDoesNotExist(
    const char* what)
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

  Security& m_self;

  static boost::atomic<InstanceId> m_nextInstanceId;
  const InstanceId m_instanceId;

  MarketDataSource& m_source;

  boost::array<boost::shared_ptr<RiskControlSymbolContext>,
               numberOfTradingModes>
      m_riskControlContext;

  const uint8_t m_pricePrecision;
  const uintmax_t m_pricePrecisionPower;
  const size_t m_numberOfItemsPerQty;
  const Qty m_lotSize;

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

  StartedBarsMutex m_startedBarsMutex;
  StartedBars m_startedBars;

  Implementation(Security& self,
                 MarketDataSource& source,
                 const Symbol& symbol,
                 const SupportedLevel1Types& supportedLevel1Types)
      : m_self(self),
        m_instanceId(m_nextInstanceId++),
        m_source(source),
        m_pricePrecision(GetPrecisionBySymbol(symbol)),
        m_pricePrecisionPower(static_cast<decltype(m_pricePrecisionPower)>(
            std::pow(10, m_pricePrecision))),
        m_numberOfItemsPerQty(GetNumberOfItemsPerQtyBySymbol(symbol)),
        m_lotSize(GetLotSizeBySymbol(symbol)),
        m_marketDataTime(0),
        m_numberOfMarketDataUpdates(0),
        m_isLevel1Started(false),
        m_supportedLevel1Types(supportedLevel1Types),
        m_isOnline(false),
        m_isOpened(false),
        m_isContractSwitchingActive(false),
        m_request({}),
        m_marketDataLog(m_source.GetContext()) {
    static_assert(numberOfTradingModes == 3, "List changed.");
    for (size_t i = 0; i < m_riskControlContext.size(); ++i) {
      m_riskControlContext[i] = m_source.GetContext()
                                    .GetRiskControl(static_cast<TradingMode>(i))
                                    .CreateSymbolContext(symbol);
    }

    for (auto& item : m_level1) {
      Unset(item);
    }
  }

  void CheckMarketDataUpdate(const pt::ptime& time) {
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
            time.date() + m_self.GetContext()
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

  bool AddLevel1Tick(const pt::ptime& time,
                     const Level1TickValue& tick,
                     const Milestones& delayMeasurement,
                     const bool flush,
                     const bool isPreviouslyChanged) {
    const auto isChanged =
        SetLevel1(time, tick, delayMeasurement, flush, isPreviouslyChanged);
    CheckMarketDataUpdate(time);
    if (CheckLevel1Start()) {
      m_level1TickSignal(time, tick, delayMeasurement, flush);
    }
    return isChanged;
  }

  bool SetLevel1(const pt::ptime& time,
                 const Level1TickValue& tick,
                 const Milestones& delayMeasurement,
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

  void FlushLevel1Update(const pt::ptime& time,
                         const Milestones& delayMeasurement,
                         bool isChanged) {
    delayMeasurement.Measure(SM_DISPATCHING_DATA_STORE);
    if (!isChanged) {
      return;
    }
    CheckMarketDataUpdate(time);
    if (CheckLevel1Start()) {
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
    m_source.GetContext().GetLog().Debug("\"%1%\" Level 1 started.", m_self);
    m_isLevel1Started = true;
    return true;
  }

  pt::ptime GetLastMarketDataTime() const {
    const auto marketDataTime = m_marketDataTime.load();
    return marketDataTime ? ConvertToPTimeFromMicroseconds(m_marketDataTime)
                          : pt::not_a_date_time;
  }

  template <Level1TickType tick>
  Double CheckAndGetLevel1Value(const Level1& level1) const {
    const double value = level1[tick];
    if (!IsSet(value)) {
      boost::format message(
          R"(Market data value "%1%" does not exist for "%2%")");
      message % ConvertToPch(tick) % m_self;
      throw MarketDataValueDoesNotExist(message.str().c_str());
    }
    AssertLe(0, value);
    return value;
  }

  void StartMarketDataLog(size_t sourceIndex) {
    auto path = m_self.GetContext().GetSettings().GetLogsDir();
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

    m_self.GetContext().GetLog().Debug("Market data log for \"%1%\": %2%.",
                                       m_self, path);
  }
};

boost::atomic<Security::InstanceId> Security::Implementation::m_nextInstanceId(
    0);

//////////////////////////////////////////////////////////////////////////

Security::Security(Context& context,
                   const Symbol& symbol,
                   MarketDataSource& source,
                   const SupportedLevel1Types& supportedLevel1Types)
    : Base(context, symbol),
      m_pimpl(new Implementation(*this, source, symbol, supportedLevel1Types)) {
  if (GetContext().GetSettings().IsMarketDataLogEnabled()) {
    m_pimpl->StartMarketDataLog(GetSource().GetIndex());
  }
}

Security::~Security() = default;

const Security::InstanceId& Security::GetInstanceId() const {
  return m_pimpl->m_instanceId;
}

RiskControlSymbolContext& Security::GetRiskControlContext(
    const TradingMode& mode) {
  AssertLt(0, mode);
  AssertGe(m_pimpl->m_riskControlContext.size(), static_cast<size_t>(mode));
  // If context is not set - risk control is disabled and nobody should call
  // this method:
  Assert(m_pimpl->m_riskControlContext[mode - 1]);
  return *m_pimpl->m_riskControlContext[mode - 1];
}

const MarketDataSource& Security::GetSource() const {
  return m_pimpl->m_source;
}

size_t Security::GetNumberOfItemsPerQty() const {
  return m_pimpl->m_numberOfItemsPerQty;
}

const Qty& Security::GetLotSize() const { return m_pimpl->m_lotSize; }

uintmax_t Security::GetPricePrecisionPower() const {
  return m_pimpl->m_pricePrecisionPower;
}

uint8_t Security::GetPricePrecision() const noexcept {
  return m_pimpl->m_pricePrecision;
}

pt::ptime Security::GetLastMarketDataTime() const {
  return m_pimpl->GetLastMarketDataTime();
}

size_t Security::TakeNumberOfMarketDataUpdates() const {
  return m_pimpl->m_numberOfMarketDataUpdates.exchange(0);
}

bool Security::IsOnline() const { return m_pimpl->m_isOnline; }

bool Security::SetOnline(const pt::ptime& time, bool isOnline) {
  if (m_pimpl->m_isOnline == isOnline) {
    return false;
  }
  GetContext().GetLog().Debug(
      "\"%1%\" now is %2% by the event %3%. Last data time: %4%.", *this,
      isOnline ? "online" : "offline", time, GetLastMarketDataTime());
  {
    const auto lock = GetSource().GetContext().SyncDispatching();
    m_pimpl->m_isOnline = isOnline;
    m_pimpl->m_serviceEventSignal(
        time, isOnline ? SERVICE_EVENT_ONLINE : SERVICE_EVENT_OFFLINE);
  }
  return true;
}

bool Security::IsTradingSessionOpened() const { return m_pimpl->m_isOpened; }

void Security::SetTradingSessionState(const pt::ptime& time, bool isOpened) {
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
    m_pimpl->m_serviceEventSignal(
        time, isOpened ? SERVICE_EVENT_TRADING_SESSION_OPENED
                       : SERVICE_EVENT_TRADING_SESSION_CLOSED);
  }
}

void Security::SwitchTradingSession(const pt::ptime& time) {
  GetContext().GetLog().Info(
      "%1% trading session is switched (%2%) by the event at %3%."
      " Last data time: %4%.",
      *this, m_pimpl->m_isOpened ? "opened" : "closed", time,
      GetLastMarketDataTime());

  {
    const auto lock = GetSource().GetContext().SyncDispatching();

    m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
    try {
      m_pimpl->m_serviceEventSignal(
          time, m_pimpl->m_isOpened ? SERVICE_EVENT_TRADING_SESSION_CLOSED
                                    : SERVICE_EVENT_TRADING_SESSION_OPENED);
    } catch (...) {
      m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
      throw;
    }

    m_pimpl->m_isOpened = !m_pimpl->m_isOpened;
    m_pimpl->m_serviceEventSignal(
        time, m_pimpl->m_isOpened ? SERVICE_EVENT_TRADING_SESSION_OPENED
                                  : SERVICE_EVENT_TRADING_SESSION_CLOSED);
  }
}

void Security::SetRequest(const Request& request) {
  m_pimpl->m_request.Merge(request);
}

const Security::Request& Security::GetRequest() const {
  return m_pimpl->m_request;
}

Price Security::GetLastPrice() const {
  return m_pimpl->CheckAndGetLevel1Value<LEVEL1_TICK_LAST_PRICE>(
      m_pimpl->m_level1);
}

Qty Security::GetLastQty() const {
  return m_pimpl->CheckAndGetLevel1Value<LEVEL1_TICK_LAST_QTY>(
      m_pimpl->m_level1);
}

Qty Security::GetTradedVolume() const {
  return m_pimpl->CheckAndGetLevel1Value<LEVEL1_TICK_TRADING_VOLUME>(
      m_pimpl->m_level1);
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
  return m_pimpl->CheckAndGetLevel1Value<LEVEL1_TICK_BID_PRICE>(
      m_pimpl->m_level1);
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
    const ContractSwitchingSlot& slot) const {
  return m_pimpl->m_contractSwitchedSignal.connect(slot);
}

Security::Level1UpdateSlotConnection Security::SubscribeToLevel1Updates(
    const Level1UpdateSlot& slot) const {
  return m_pimpl->m_level1UpdateSignal.connect(slot);
}

Security::Level1UpdateSlotConnection Security::SubscribeToLevel1Ticks(
    const Level1TickSlot& slot) const {
  return m_pimpl->m_level1TickSignal.connect(slot);
}

Security::NewTradeSlotConnection Security::SubscribeToTrades(
    const NewTradeSlot& slot) const {
  return m_pimpl->m_tradeSignal.connect(slot);
}

Security::NewTradeSlotConnection Security::SubscribeToBrokerPositionUpdates(
    const BrokerPositionUpdateSlot& slot) const {
  return m_pimpl->m_brokerPositionUpdateSignal.connect(slot);
}

Security::NewBarSlotConnection Security::SubscribeToBars(
    const NewBarSlot& slot) const {
  return m_pimpl->m_barSignal.connect(slot);
}

Security::BookUpdateTickSlotConnection Security::SubscribeToBookUpdateTicks(
    const BookUpdateTickSlot& slot) const {
  return m_pimpl->m_bookUpdateTickSignal.connect(slot);
}

Security::ServiceEventSlotConnection Security::SubscribeToServiceEvents(
    const ServiceEventSlot& slot) const {
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

void Security::SetLevel1(const pt::ptime& time,
                         const Level1TickValue& tick,
                         const Milestones& delayMeasurement) {
  SetLevel1(time, tick, true, false, delayMeasurement);
}

bool Security::SetLevel1(const pt::ptime& time,
                         const Level1TickValue& tick,
                         bool flush,
                         bool isPreviouslyChanged,
                         const Milestones& delayMeasurement) {
  if (!m_pimpl->SetLevel1(time, tick, delayMeasurement, flush,
                          isPreviouslyChanged)) {
    return false;
  }
  GetContext().InvokeDropCopy([this, &time, &tick](DropCopy& dropCopy) {
    dropCopy.CopyLevel1(*this, time, tick);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick);
  return true;
}

void Security::SetLevel1(const pt::ptime& time,
                         const Level1TickValue& tick1,
                         const Level1TickValue& tick2,
                         const Milestones& delayMeasurement) {
  AssertNe(tick1.GetType(), tick2.GetType());
  if (!m_pimpl->SetLevel1(
          time, tick2, delayMeasurement, true,
          m_pimpl->SetLevel1(time, tick1, delayMeasurement, false, false))) {
    return;
  }
  GetContext().InvokeDropCopy(
      [this, &time, &tick1, &tick2](DropCopy& dropCopy) {
        dropCopy.CopyLevel1(*this, time, tick1, tick2);
      });
  m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick1, tick2);
}

void Security::SetLevel1(const pt::ptime& time,
                         const Level1TickValue& tick1,
                         const Level1TickValue& tick2,
                         const Level1TickValue& tick3,
                         const Milestones& delayMeasurement) {
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
      [this, &time, &tick1, &tick2, &tick3](DropCopy& dropCopy) {
        dropCopy.CopyLevel1(*this, time, tick1, tick2, tick3);
      });
  m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick1, tick2, tick3);
}

void Security::SetLevel1(const pt::ptime& time,
                         const Level1TickValue& tick1,
                         const Level1TickValue& tick2,
                         const Level1TickValue& tick3,
                         const Level1TickValue& tick4,
                         const Milestones& delayMeasurement) {
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
      [this, &time, &tick1, &tick2, &tick3, &tick4](DropCopy& dropCopy) {
        dropCopy.CopyLevel1(*this, time, tick1, tick2, tick3, tick4);
      });
  m_pimpl->m_marketDataLog.WriteLevel1Update(time, tick1, tick2, tick3, tick4);
}

void Security::SetLevel1(const pt::ptime& time,
                         const std::vector<Level1TickValue>& ticks,
                         const Milestones& delayMeasurement) {
  size_t counter = 0;
  bool isPreviousChanged = false;
  for (const auto& tick : ticks) {
    isPreviousChanged =
        m_pimpl->SetLevel1(time, tick, delayMeasurement,
                           ++counter >= ticks.size(), isPreviousChanged);
  }
  if (!isPreviousChanged) {
    return;
  }
  GetContext().InvokeDropCopy([this, &time, &ticks](DropCopy& dropCopy) {
    dropCopy.CopyLevel1(*this, time, ticks);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Update(time, ticks);
}

bool Security::AddLevel1Tick(const pt::ptime& time,
                             const Level1TickValue& tick,
                             bool flush,
                             bool isPreviouslyChanged,
                             const Milestones& delayMeasurement) {
  const bool result = m_pimpl->AddLevel1Tick(time, tick, delayMeasurement,
                                             flush, isPreviouslyChanged);
  GetContext().InvokeDropCopy([this, &time, &tick](DropCopy& dropCopy) {
    dropCopy.CopyLevel1(*this, time, tick);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, tick);
  return result;
}

void Security::AddLevel1Tick(const pt::ptime& time,
                             const Level1TickValue& tick,
                             const Milestones& delayMeasurement) {
  m_pimpl->AddLevel1Tick(time, tick, delayMeasurement, true, false);
  GetContext().InvokeDropCopy(
      [&](DropCopy& dropCopy) { dropCopy.CopyLevel1(*this, time, tick); });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, tick);
}

void Security::AddLevel1Tick(const pt::ptime& time,
                             const Level1TickValue& tick1,
                             const Level1TickValue& tick2,
                             const Milestones& delayMeasurement) {
  m_pimpl->AddLevel1Tick(
      time, tick2, delayMeasurement, true,
      m_pimpl->AddLevel1Tick(time, tick1, delayMeasurement, false, false));
  GetContext().InvokeDropCopy([&](DropCopy& dropCopy) {
    dropCopy.CopyLevel1(*this, time, tick1, tick2);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, tick1, tick2);
}

void Security::AddLevel1Tick(const pt::ptime& time,
                             const Level1TickValue& tick1,
                             const Level1TickValue& tick2,
                             const Level1TickValue& tick3,
                             const Milestones& delayMeasurement) {
  m_pimpl->AddLevel1Tick(
      time, tick3, delayMeasurement, true,
      m_pimpl->AddLevel1Tick(
          time, tick2, delayMeasurement, false,
          m_pimpl->AddLevel1Tick(time, tick1, delayMeasurement, false, false)));
  GetContext().InvokeDropCopy([&](DropCopy& dropCopy) {
    dropCopy.CopyLevel1(*this, time, tick1, tick2, tick3);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, tick1, tick2, tick3);
}

void Security::AddLevel1Tick(const pt::ptime& time,
                             const Level1TickValue& tick1,
                             const Level1TickValue& tick2,
                             const Level1TickValue& tick3,
                             const Level1TickValue& tick4,
                             const Milestones& delayMeasurement) {
  m_pimpl->AddLevel1Tick(
      time, tick4, delayMeasurement, true,
      m_pimpl->AddLevel1Tick(
          time, tick3, delayMeasurement, false,
          m_pimpl->AddLevel1Tick(
              time, tick2, delayMeasurement, false,
              m_pimpl->AddLevel1Tick(time, tick1, delayMeasurement, false,
                                     false))));
  GetContext().InvokeDropCopy([&](DropCopy& dropCopy) {
    dropCopy.CopyLevel1(*this, time, tick1, tick2, tick3, tick4);
  });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, tick1, tick2, tick4);
}

void Security::AddLevel1Tick(const pt::ptime& time,
                             const std::vector<Level1TickValue>& ticks,
                             const Milestones& delayMeasurement) {
  size_t counter = 0;
  bool isPreviousChanged = false;
  for (const auto& tick : ticks) {
    isPreviousChanged =
        m_pimpl->AddLevel1Tick(time, tick, delayMeasurement,
                               ++counter >= ticks.size(), isPreviousChanged);
  }
  GetContext().InvokeDropCopy(
      [&](DropCopy& dropCopy) { dropCopy.CopyLevel1(*this, time, ticks); });
  m_pimpl->m_marketDataLog.WriteLevel1Tick(time, ticks);
}

void Security::AddTrade(const pt::ptime& time,
                        const Price& price,
                        const Qty& qty,
                        const Milestones& delayMeasurement,
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

void Security::UpdateBar(const Bar& bar) {
  m_pimpl->m_barSignal(bar);
  GetContext().InvokeDropCopy(
      [this, &bar](DropCopy& dropCopy) { dropCopy.CopyBar(*this, bar); });
}

void Security::SetBrokerPosition(bool isLong,
                                 const Qty& qty,
                                 const Volume& volume,
                                 bool isInitial) {
  m_pimpl->m_brokerPositionUpdateSignal(isLong, qty, volume, isInitial);
}

void Security::SetBook(PriceBook& book, const Milestones& delayMeasurement) {
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
      [this, &book](DropCopy& dropCopy) { dropCopy.CopyBook(*this, book); });

  // Adjusting:
  while (!book.GetBid().IsEmpty() && !book.GetAsk().IsEmpty() &&
         book.GetBid().GetTop().GetPrice() >
             book.GetAsk().GetTop().GetPrice()) {
    const auto isBidOlder =
        book.GetBid().GetTop().GetTime() < book.GetAsk().GetTop().GetTime();
    isBidOlder ? book.GetBid().PopTop() : book.GetAsk().PopTop();
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

const ContractExpiration& Security::GetExpiration() const {
  if (!m_pimpl->m_expiration) {
    boost::format error("%1% doesn't have expiration");
    error % *this;
    throw LogicError(error.str().c_str());
  }
  return *m_pimpl->m_expiration;
}

bool Security::HasExpiration() const {
  return static_cast<bool>(m_pimpl->m_expiration);
}

bool Security::SetExpiration(const pt::ptime& time,
                             const ContractExpiration& expiration) {
  return SetExpiration(time,
                       std::forward<const ContractExpiration>(expiration));
}

bool Security::SetExpiration(const pt::ptime& time,
                             const ContractExpiration&& expiration) {
#ifdef BOOST_ENABLE_ASSERT_HANDLER
  if (m_pimpl->m_expiration) {
    AssertLt(*m_pimpl->m_expiration, expiration);
  }
#endif

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
    auto isSwitched = true;
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
    m_pimpl->m_expiration = expiration;
    for (auto& item : m_pimpl->m_level1) {
      Unset(item);
    }
    m_pimpl->m_marketDataTime = 0;

    if (request.IsEarlier(m_pimpl->m_request)) {
      m_pimpl->m_request.Merge(request);
      return true;
    }
    return false;
  }
}

void Security::ContinueContractSwitchingToNextExpiration() {
  Assert(m_pimpl->m_isContractSwitchingActive);
  GetSource().SwitchToNextContract(*this);
}

void Security::StartBars(const pt::ptime& startTime,
                         const pt::time_duration& barSize) {
  const StartedBarsWriteLock lock(m_pimpl->m_startedBarsMutex);
  const auto result =
      m_pimpl->m_startedBars.emplace(barSize, std::make_pair(startTime, 1));
  if (result.second) {
    return;
  }
  auto& bar = result.first->second;
  if (bar.first > startTime) {
    bar.first = startTime;
  }
  ++bar.second;
}

void Security::StopBars(const pt::time_duration& barSize) {
  const StartedBarsWriteLock lock(m_pimpl->m_startedBarsMutex);
  const auto& it = m_pimpl->m_startedBars.find(barSize);
  Assert(it != m_pimpl->m_startedBars.cend());
  if (it == m_pimpl->m_startedBars.cend()) {
    return;
  }
  auto& refCounter = it->second.second;
  AssertLt(0, refCounter);
  if (--refCounter) {
    return;
  }
  m_pimpl->m_startedBars.erase(it);
}

void Security::SetBarsStartTime(const pt::time_duration& barSize,
                                const pt::ptime& startTime) {
  const StartedBarsWriteLock lock(m_pimpl->m_startedBarsMutex);
  const auto& it = m_pimpl->m_startedBars.find(barSize);
  if (it == m_pimpl->m_startedBars.cend()) {
    return;
  }
  it->second.first = startTime;
}

std::vector<std::pair<pt::time_duration, pt::ptime>> Security::GetStartedBars()
    const {
  std::vector<std::pair<pt::time_duration, pt::ptime>> result;
  {
    const StartedBarsReadLock lock(m_pimpl->m_startedBarsMutex);
    result.reserve(m_pimpl->m_startedBars.size());
    for (const auto& bar : m_pimpl->m_startedBars) {
      result.emplace_back(std::make_pair(bar.first, bar.second.first));
    }
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////

std::ostream& trdk::operator<<(std::ostream& os, const Security& security) {
  return os << static_cast<const Security::Base&>(security) << '.'
            << security.GetSource().GetInstanceName();
}

////////////////////////////////////////////////////////////////////////////////
