/**************************************************************************
 *   Created: May 26, 2012 8:29:56 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "IbClient.hpp"
#include "Core/Settings.hpp"
#include "IbTradingSystem.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::InteractiveBrokers;

namespace ib = trdk::Interaction::InteractiveBrokers;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;
namespace fs = boost::filesystem;

///////////////////////////////////////////////////////////////////////////////

namespace {

const pt::time_duration pingRequestPeriod = pt::seconds(120);
const pt::time_duration timeout = pt::seconds(10);
const pt::time_duration maxIterationTime = pt::milliseconds(10);

//////////////////////////////////////////////////////////////////////////

class ConvertToIbQtyTypeProxy {
 public:
  explicit ConvertToIbQtyTypeProxy(const Qty &source) : m_source(&source) {}

 public:
  template <typename Result>
  operator Result() const {
    return Result(*m_source);
  }

 private:
  const Qty *m_source;
};

ConvertToIbQtyTypeProxy ConvertToIbType(const Qty &source) {
  return ConvertToIbQtyTypeProxy(source);
}

//////////////////////////////////////////////////////////////////////////

void FormatOrderDateTime(const pt::ptime &dateTime, IBString &destination) {
  // Format: 20060505 08:00:00 {time zone}
  boost::format result("%d%02d%02d %02d:%02d:%02d UTC");
  {
    const auto &date = dateTime.date();
    result % date.year() % date.month().as_number() % date.day();
  }
  {
    const auto &time = dateTime.time_of_day();
    result % time.hours() % time.minutes() % time.seconds();
  }
  destination = result.str();
}

////////////////////////////////////////////////////////////////////////////////

IBString FormatLocalFuturesSymbol(const std::string &symbol,
                                  const ContractExpiration &expirationInfo) {
  return symbol + expirationInfo.GetContract(true);
}

////////////////////////////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////

Client::Client(ib::TradingSystem &ts,
               bool isNoHistoryMode,
               int clientId,
               const std::string &host,
               unsigned short port)
    : m_ts(ts),
      m_isNoHistoryMode(isNoHistoryMode),
      m_host(host),
      m_port(port),
      m_clientId(clientId),
      m_connectionState(CONNECTION_STATE_NOT_CONNECTED),
      m_state(PING_STATE_REQ),
      m_thread(nullptr),
      m_orderStatusesMap(GetOrderStatusesMap()),
      m_seqNumber(-1),
      m_accountInfo(nullptr),
      m_securityInSwitching(nullptr) {
  m_client.reset(new EPosixClientSocket(this));
  LogConnectionAttempt();
  const bool connectResult =
      m_client->eConnect(m_host.c_str(), m_port, m_clientId);
  AssertEq(connectResult, m_client->isConnected());
  if (!connectResult || !m_client->isConnected()) {
    throw trdk::Interactor::ConnectError("Failed to connect to TWS");
  }
  m_connectionState = CONNECTION_STATE_CONNECTED;
  LogConnect();
}

Client::~Client() {
  try {
    const std::string host = m_host;
    const auto port = m_port;
    const auto clientId = m_clientId;

    LogDisconnectAttempt();

    if (m_thread) {
      {
        const Lock lock(m_mutex);
        m_connectionState = CONNECTION_STATE_NOT_CONNECTED;
      }
      m_condition.notify_all();
      m_thread->join();
      delete m_thread;
    }

    m_client->eDisconnect();
    m_client.reset();

    m_ts.GetTsLog().Info(
        "Connection with \"%1%:%2%\" (client ID %3%) is closed.", host, port,
        clientId);

  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void Client::Task() {
  m_ts.GetTsLog().Debug("Started client task.");
  bool isInited = false;
  pt::ptime nextIterationTime = boost::get_system_time() + maxIterationTime;
  size_t heavyCount = 0;
  for (Lock lock(m_mutex);;) {
    try {
      m_clientNow = boost::get_system_time();
      if (isInited) {
        if (!m_connectionState) {
          break;
        }
        if (nextIterationTime > m_clientNow) {
          heavyCount = 0;
          m_condition.timed_wait(lock, nextIterationTime);
        } else if (heavyCount == 5 ||
                   (heavyCount > 5 && !(++heavyCount % 10))) {
          lock.unlock();
          m_ts.GetTsLog().Warn(
              "Connection task is heavily loaded"
              " (iterations without sleep: %1%).",
              heavyCount);
          lock.lock();
        }
        m_clientNow = boost::get_system_time();
      }
      nextIterationTime = m_clientNow + maxIterationTime;
      CheckTimeout();
      while (m_connectionState && ProcessMessages()) {
        if (m_callBackList.size() > 0) {
          OrderCallbackList callBackList;
          callBackList.swap(m_callBackList);
          lock.unlock();
          std::for_each(
              callBackList.begin(), callBackList.end(),
              [](OrderCallbackList::value_type &callBack) { callBack(); });
          lock.lock();
        } else {
          lock.unlock();
          lock.lock();
        }
      }
      if (!isInited) {
        isInited = m_seqNumber >= 0;
        if (isInited) {
          m_condition.notify_all();
        } else {
          lock.unlock();
          m_ts.GetTsLog().Debug("Waiting for seqnumber...");
          boost::this_thread::sleep(pt::milliseconds(500));
          lock.lock();
        }
      }
      if (!m_connectionState) {
        break;
      }
    } catch (...) {
      lock.unlock();
      AssertFailNoException();
      throw;
    }
  }
  m_ts.GetTsLog().Debug("Client task finished.");
}

Contract Client::GetContract(
    const trdk::Security &security,
    const ContractExpiration *customContractExpiration) const {
  Contract contract;
  static_assert(numberOfSecurityTypes == 6, "Security type list changed.");
  const Symbol &symbol = security.GetSymbol();
  switch (symbol.GetSecurityType()) {
    case SECURITY_TYPE_STOCK:
      Assert(!customContractExpiration);
      contract.secType = "STK";
      contract.symbol = symbol.GetSymbol();
      contract.primaryExchange = symbol.GetPrimaryExchange();
      break;
    case SECURITY_TYPE_FUTURES:
      contract.secType = "FUT";
      contract.includeExpired = true;
      contract.localSymbol =
          !symbol.IsExplicit()
              ? FormatLocalFuturesSymbol(symbol.GetSymbol(),
                                         customContractExpiration
                                             ? *customContractExpiration
                                             : security.GetExpiration())
              : symbol.GetSymbol();
      break;
    case SECURITY_TYPE_FUTURES_OPTIONS:
      Assert(!customContractExpiration);
      contract.secType = "FOP";
      contract.symbol = symbol.GetSymbol();
      contract.strike = symbol.GetStrike();
      contract.right = symbol.GetRightAsString();
      break;
    case SECURITY_TYPE_OPTIONS:
      Assert(!customContractExpiration);
      Assert(security.HasExpiration());
      contract.secType = "OPT";
      contract.symbol = symbol.GetSymbol();
      contract.strike = symbol.GetStrike();
      contract.right = symbol.GetRightAsString();
      contract.expiry = gr::to_iso_string(security.GetExpiration().GetDate());
      break;
    default:
      throw MethodDoesNotImplementedError("Security type is not supported");
      break;
  }
  contract.currency = ConvertToIso(symbol.GetCurrency());
  contract.exchange = symbol.GetExchange();
  return contract;
}

Contract Client::GetContract(const trdk::Security &security,
                             const OrderParams &params) const {
  return GetContract(security, params.expiration);
}

void Client::StartData() {
  Lock lock(m_mutex);
  Assert(!m_thread);
  AssertEq(CONNECTION_STATE_CONNECTED, m_connectionState);
  if (m_thread) {
    return;
  }

  if (m_accountInfo) {
    m_ts.GetMdsLog().Info("Requesting account updates for \"%1%\"...",
                          m_account);
    m_client->reqAccountUpdates(true, m_account);
  }

  bool isBrokerPositionsRequred = m_ts.m_positions ? true : false;
  if (!isBrokerPositionsRequred) {
    foreach (const auto &security, m_ts.m_securities) {
      if (security->IsBrokerPositionRequired()) {
        isBrokerPositionsRequred = true;
        break;
      }
    }
  }
  if (isBrokerPositionsRequred) {
    m_ts.GetMdsLog().Info("Requesting positions info...");
    m_client->reqPositions();
  } else {
    m_connectionState = CONNECTION_STATE_READY;
  }

  m_thread = new boost::thread([this]() { Task(); });
  if (!m_condition.timed_wait(lock, timeout)) {
    m_ts.GetMdsLog().Error("No seqnumber received.");
    m_connectionState = CONNECTION_STATE_NOT_CONNECTED;
    m_condition.notify_all();
    throw trdk::TradingSystem::ConnectError("No seqnumber received");
  }
}

bool Client::ProcessMessages() {
  if (m_client->fd() < 0) {
    return false;
  }

  fd_set readSet;
  fd_set writeSet;
  fd_set errorSet;

#ifdef _WINDOWS
#pragma warning(push)
#pragma warning(disable : 4389)
#pragma warning(disable : 4127)
#endif

  FD_ZERO(&readSet);
  FD_SET(m_client->fd(), &readSet);

  FD_ZERO(&writeSet);
  if (!m_client->isOutBufferEmpty()) {
    FD_SET(m_client->fd(), &writeSet);
  }

  FD_ZERO(&errorSet);
  FD_CLR(m_client->fd(), &errorSet);

#ifdef _WINDOWS
#pragma warning(pop)
#endif

  timeval selectWaitTime = {};
  const int selectResult = select(m_client->fd() + 1, &readSet, &writeSet,
                                  &errorSet, &selectWaitTime);
  if (selectResult == 0) {  // timeout
    return false;
  } else if (selectResult < 0) {
    m_ts.GetTsLog().Debug("Connection process operation returned DISCONNECT.");
    m_connectionState = CONNECTION_STATE_NOT_CONNECTED;
    return false;
  } else if (m_client->fd() < 0) {
    return false;
  }

  if (FD_ISSET(m_client->fd(), &errorSet)) {
    m_client->onError();
  }
  if (m_client->fd() >= 0 && FD_ISSET(m_client->fd(), &writeSet)) {
    m_client->onSend();
  }
  if (m_client->fd() >= 0 && FD_ISSET(m_client->fd(), &readSet)) {
    UpdateLastResponseTime();
    m_client->onReceive();
  }

  return true;
}

void Client::Subscribe(const OrderStatusSlot &orderStatusSlot) {
  m_orderStatusSignal.connect(orderStatusSlot);
}

void Client::SubscribeToMarketData(ib::Security &security) {
  if (security.IsTradesRequired() && !security.IsTestSource()) {
    throw trdk::TradingSystem::Error(
        "Interactive Brokers doesn't provide trades info");
  } else if (!security.IsLevel1Required() && !security.IsBarsRequired() &&
             !(security.IsTestSource() && security.IsTradesRequired())) {
    return;
  }

  const Lock lock(m_mutex);
  if (IsSubscribed(m_marketDataRequests, m_postponedMarketDataRequests,
                   security)) {
    return;
  }
  CheckState();

  m_connectionState < CONNECTION_STATE_READY
      ? PostponeMarketDataSubscription(security)
      : DoMarketDataSubscription(security);
}

void Client::PostponeMarketDataSubscription(ib::Security &security) const {
  Assert(std::find(m_postponedMarketDataRequests.begin(),
                   m_postponedMarketDataRequests.end(),
                   &security) == m_postponedMarketDataRequests.end());
  m_postponedMarketDataRequests.push_back(&security);
}

void Client::FlushPostponedMarketDataSubscription() {
  AssertLe(CONNECTION_STATE_READY, m_connectionState);
  while (!m_postponedMarketDataRequests.empty()) {
    DoMarketDataSubscription(**m_postponedMarketDataRequests.begin());
    m_postponedMarketDataRequests.pop_front();
  }
}

void Client::DoMarketDataSubscription(ib::Security &security) {
  Assert(!IsSubscribed(m_marketDataRequests, security));
  Assert(security.IsLevel1Required() || security.IsBarsRequired() ||
         (security.IsTestSource() && security.IsTradesRequired()));
  if (m_isNoHistoryMode || !SendMarketDataHistoryRequest(security)) {
    SendMarketDataRequest(security);
  }
}

void Client::SendMarketDataRequest(ib::Security &security) {
  Assert(!m_mutex.try_lock());
  Assert(security.IsLevel1Required() || security.IsBarsRequired() ||
         (security.IsTestSource() && security.IsTradesRequired()));
  Assert(!IsSubscribed(m_marketDataRequests, security));

  if (security.IsLevel1Required() ||
      (security.IsTestSource() &&
       (security.IsBarsRequired() || security.IsTradesRequired()))) {
    const SecurityRequest request(security, TakeTickerId());
    const auto &contract = GetContract(*request.security);

    auto requests(m_marketDataRequests);
    Verify(requests.insert(request).second);

    std::list<IBString> genericTicklist;
    genericTicklist.push_back("233");

    m_client->reqMktData(request.tickerId, contract,
                         boost::join(genericTicklist, ","), false,
                         TagValueListSPtr());

    m_ts.GetMdsLog().Info(
        "Sent Level I market data subscription request for \"%1%\""
        " (symbol \"%2%\", ticker ID: %3%).",
        *request.security, (!contract.localSymbol.empty() ? contract.localSymbol
                                                          : contract.symbol),
        request.tickerId);

    if (!request.security->IsOnline()) {
      request.security->SetOnline(pt::not_a_date_time, true);
    } else {
      Assert(m_securityInSwitching);
    }

    requests.swap(m_marketDataRequests);
  }

  if (security.IsBarsRequired()) {
    const char *const whatToShowList[] = {"TRADES", "BID",
                                          "ASK" /*, MIDPOINT*/};
    foreach (const char *const whatToShow, whatToShowList) {
      const SecurityRequest request(security, TakeTickerId());
      const auto &contract = GetContract(*request.security);

      auto requests(m_barsRequest);
      requests.insert(request);

      m_client->reqRealTimeBars(
          request.tickerId, contract,
          // Currently only 5 second bars are supported, if any other
          // value is used, an exception will be thrown.
          5, whatToShow, false, TagValueListSPtr());

      m_ts.GetMdsLog().Info(
          "Sent Real Time Bars (%1%) subscription request for \"%2%\""
          " (ticker ID: %3%).",
          whatToShow, *request.security, request.tickerId);

      if (!request.security->IsOnline()) {
        request.security->SetOnline(pt::not_a_date_time, true);
      }

      requests.swap(m_barsRequest);
    }
  }
}

bool Client::SendMarketDataHistoryRequest(ib::Security &security) {
  Assert(!m_isNoHistoryMode);
  const auto &requestedDataStartTime = security.GetRequest().GetTime();
  if (requestedDataStartTime == pt::not_a_date_time) {
    return false;
  }
  return SendMarketDataHistoryRequest(security, requestedDataStartTime,
                                      m_ts.GetContext().GetCurrentTime(), 0,
                                      .0);
}

bool Client::SendMarketDataHistoryRequest(ib::Security &security,
                                          const pt::ptime &requestStart,
                                          const pt::ptime &requestEnd,
                                          size_t numberOfPrevRequests,
                                          double prevClosePrice) {
  if (requestStart >= requestEnd) {
    return false;
  }
  const pt::ptime &now = m_ts.GetContext().GetCurrentTime();
  if (now <= requestStart) {
    return false;
  }

  SecurityHistoryRequest request(security, TakeTickerId(), numberOfPrevRequests,
                                 requestStart, requestEnd);

  auto contract = GetContract(*request.security);

  request.prevClosePrice = prevClosePrice;
  if (request.numberOfPrevRequests >= 6) {
    // Making six or more historical data requests for the same Contract,
    // Exchange and Tick Type within two seconds
    // @sa
    // https://www.interactivebrokers.com/en/software/api/apiguide/tables/historical_data_limitations.htm
    boost::this_thread::sleep(pt::seconds(2));
    request.numberOfPrevRequests = 1;
  }

  if (security.GetSymbol().GetSecurityType() == SECURITY_TYPE_FUTURES) {
    for (request.expiration = m_ts.GetContext().GetExpirationCalendar().Find(
             request.security->GetSymbol(), request.subRequestEnd.date());
         ;) {
      if (!request.expiration) {
        boost::format error(
            "Failed to find expiration info for \"%1%\" and %2%");
        error % *request.security % requestStart;
        throw trdk::MarketDataSource::Error(error.str().c_str());
      }
      contract.localSymbol = FormatLocalFuturesSymbol(
          request.security->GetSymbol().GetSymbol(), *request.expiration);
      const auto &prevExpiration = std::prev(request.expiration);
      if (prevExpiration) {
        request.subRequestStart = pt::ptime(prevExpiration->GetDate());
        request.subRequestStart += pt::hours(24);
        if (request.subRequestStart >= request.subRequestEnd) {
          AssertEq(request.subRequestStart, request.subRequestEnd);
          --request.expiration;
          continue;
        }
        break;
      }
    }
  }

  {
    const auto maxDuration = pt::seconds(86400);
    if (request.subRequestEnd - request.subRequestStart > maxDuration) {
      request.subRequestStart = request.subRequestEnd - maxDuration;
    }
  }

  auto requests(m_historyRequest);
  Verify(requests.emplace(std::move(request)).second);

  std::ostringstream endTimeOss;
  endTimeOss << request.subRequestEnd.date().year() << std::setw(2)
             << std::setfill('0')
             << unsigned short(request.subRequestEnd.date().month())
             << std::setw(2) << std::setfill('0')
             << request.subRequestEnd.date().day() << ' ' << std::setw(2)
             << std::setfill('0') << request.subRequestEnd.time_of_day().hours()
             << ':' << std::setw(2) << std::setfill('0')
             << request.subRequestEnd.time_of_day().minutes() << ':'
             << std::setw(2) << std::setfill('0')
             << request.subRequestEnd.time_of_day().seconds();
  const auto &endTime = endTimeOss.str();

  const auto period =
      (boost::format("%1% S") %
       std::max(
           60,
           (request.subRequestEnd - request.subRequestStart).total_seconds()))
          .str();

  // @sa
  // https://www.interactivebrokers.com/en/software/api/apiguide/tables/historical_data_limitations.htm
  m_client->reqHistoricalData(
      request.tickerId, contract, endTime, period,
      // Sends as often as possible, but only for bars it doesn't make sense
      // less as 5 second as "Currently only 5 second bars are supported, if
      // any other value is used, an exception will be thrown" for real bars:
      "5 mins", "TRADES", 0, 1, TagValueListSPtr());

  {
    const char *const message =
        "Sent Level I"
        " market data history request for \"%1%\": %2% - %3%"
        " (symbol: \"%4%\""
        ", end time: \"%5%\", period: \"%6%\", ticker ID: %7%).";
    if (numberOfPrevRequests == 0) {
      m_ts.GetMdsLog().Info(
          message, *request.security, request.subRequestStart,
          request.subRequestEnd,
          (!contract.localSymbol.empty() ? contract.localSymbol
                                         : contract.symbol),
          endTime, period, request.tickerId);
    } else {
      m_ts.GetMdsLog().Debug(
          message, *request.security, request.subRequestStart,
          request.subRequestEnd,
          (!contract.localSymbol.empty() ? contract.localSymbol
                                         : contract.symbol),
          endTime, period, request.tickerId);
    }
  }

  //! @todo Implement request list clearing after finish.
  requests.swap(m_historyRequest);

  return true;
}

void Client::SubscribeToMarketDepthLevel2(ib::Security &security) {
  //! @todo support postponed Level 2 subscription

  AssertFail("Market Depth Level II not yet supported by security.");

  const Lock lock(m_mutex);
  if (IsSubscribed(m_marketDepthLevel2Requests, security)) {
    return;
  }
  CheckState();

  const SecurityRequest request(security, TakeTickerId());
  auto requests(m_marketDepthLevel2Requests);
  requests.insert(request);

  const auto &contract = GetContract(*request.security);
  m_client->reqMktDepth(request.tickerId, contract,
                        std::numeric_limits<int>::max(), TagValueListSPtr());

  m_ts.GetMdsLog().Info(
      "Sent Level II"
      " market depth subscription request for \"%1%\""
      " (symbol \"%2%\", ticker ID: %3%).",
      *request.security,
      (!contract.localSymbol.empty() ? contract.localSymbol : contract.symbol),
      request.tickerId);

  requests.swap(m_marketDepthLevel2Requests);
}

void Client::ApplyOrderParams(const OrderParams &params, Order &order) const {
  if (params.displaySize) {
    AssertLt(0, *params.displaySize);
    order.displaySize = ConvertToIbType(*params.displaySize);
  }

  Assert(!params.goodInSeconds || !params.goodTillTime);
  if (params.goodTillTime) {
    FormatOrderDateTime(*params.goodTillTime, order.goodTillDate);
    order.tif = "GTD";
  } else if (params.goodInSeconds) {
    FormatOrderDateTime(m_ts.GetContext().GetCurrentTime() +
                            pt::seconds(long(*params.goodInSeconds)),
                        order.goodTillDate);
    order.tif = "GTD";
  }
}

///////////////////////////////////////////////////////////////////////////////

trdk::OrderId Client::PlaceBuyOrder(const trdk::Security &security,
                                    Qty qty,
                                    const OrderParams &params) {
  AssertLt(0, qty);

  const auto &contract = GetContract(security, params);

  Order order;
  ApplyOrderParams(params, order);
  order.action = "BUY";
  order.totalQuantity = ConvertToIbType(qty);
  order.orderType = "MKT";

  const Lock lock(m_mutex);
  CheckState();
  const auto orderId = TakeOrderId();
  m_client->placeOrder(orderId, contract, order);
  UpdateLastRequestTime();
  m_condition.notify_all();

  return orderId;
}

trdk::OrderId Client::PlaceBuyOrder(const trdk::Security &security,
                                    Qty qty,
                                    double price,
                                    const OrderParams &params) {
  AssertLt(0, qty);

  const auto &contract = GetContract(security, params);

  Order order;
  ApplyOrderParams(params, order);
  order.action = "BUY";
  order.totalQuantity = ConvertToIbType(qty);
  order.orderType = "LMT";
  order.lmtPrice = price;

  const Lock lock(m_mutex);
  CheckState();
  const auto orderId = TakeOrderId();
  m_client->placeOrder(orderId, contract, order);
  UpdateLastRequestTime();
  m_condition.notify_all();

  return orderId;
}

trdk::OrderId Client::PlaceBuyOrderWithMarketPrice(
    const trdk::Security &security,
    Qty qty,
    double stopPrice,
    const OrderParams &params) {
  AssertLt(0, qty);

  const auto &contract = GetContract(security, params);

  Order order;
  ApplyOrderParams(params, order);
  order.action = "BUY";
  order.totalQuantity = ConvertToIbType(qty);
  order.orderType = "STP";
  order.auxPrice = stopPrice;

  const Lock lock(m_mutex);
  CheckState();
  const auto orderId = TakeOrderId();
  m_client->placeOrder(orderId, contract, order);
  UpdateLastRequestTime();
  m_condition.notify_all();

  return orderId;
}

trdk::OrderId Client::PlaceBuyIocOrder(const trdk::Security &security,
                                       Qty qty,
                                       double price,
                                       const OrderParams &params) {
  const auto &contract = GetContract(security, params);

  Order order;
  ApplyOrderParams(params, order);
  order.action = "BUY";
  order.totalQuantity = ConvertToIbType(qty);
  order.orderType = "LMT";
  AssertEq(std::string(), order.tif);
  order.tif = "IOC";
  order.lmtPrice = price;

  const Lock lock(m_mutex);
  CheckState();
  const auto orderId = TakeOrderId();
  m_client->placeOrder(orderId, contract, order);
  UpdateLastRequestTime();
  m_condition.notify_all();

  return orderId;
}

trdk::OrderId Client::PlaceSellOrder(const trdk::Security &security,
                                     Qty qty,
                                     const OrderParams &params) {
  AssertLt(0, qty);

  const auto &contract = GetContract(security, params);

  Order order;
  ApplyOrderParams(params, order);
  order.action = "SELL";
  order.totalQuantity = ConvertToIbType(qty);
  order.orderType = "MKT";

  const Lock lock(m_mutex);
  CheckState();
  const auto orderId = TakeOrderId();
  m_client->placeOrder(orderId, contract, order);
  UpdateLastRequestTime();
  m_condition.notify_all();

  return orderId;
}

trdk::OrderId Client::PlaceSellOrder(const trdk::Security &security,
                                     Qty qty,
                                     double price,
                                     const OrderParams &params) {
  AssertLt(0, qty);

  const auto &contract = GetContract(security, params);

  Order order;
  ApplyOrderParams(params, order);
  order.action = "SELL";
  order.totalQuantity = ConvertToIbType(qty);
  order.orderType = "LMT";
  order.lmtPrice = price;

  const Lock lock(m_mutex);
  CheckState();
  const auto orderId = TakeOrderId();
  m_client->placeOrder(orderId, contract, order);
  UpdateLastRequestTime();
  m_condition.notify_all();

  return orderId;
}

trdk::OrderId Client::PlaceSellOrderWithMarketPrice(
    const trdk::Security &security,
    Qty qty,
    double stopPrice,
    const OrderParams &params) {
  AssertLt(0, qty);

  const auto &contract = GetContract(security, params);

  Order order;
  ApplyOrderParams(params, order);
  order.action = "SELL";
  order.totalQuantity = ConvertToIbType(qty);
  order.orderType = "STP";
  order.auxPrice = stopPrice;

  const Lock lock(m_mutex);
  CheckState();
  const auto orderId = TakeOrderId();
  m_client->placeOrder(orderId, contract, order);
  UpdateLastRequestTime();
  m_condition.notify_all();

  return orderId;
}

trdk::OrderId Client::PlaceSellIocOrder(const trdk::Security &security,
                                        Qty qty,
                                        double price,
                                        const OrderParams &params) {
  const auto &contract = GetContract(security, params);

  Order order;
  ApplyOrderParams(params, order);
  order.action = "SELL";
  order.totalQuantity = ConvertToIbType(qty);
  order.orderType = "LMT";
  AssertEq(std::string(), order.tif);
  order.tif = "IOC";
  order.lmtPrice = price;

  const Lock lock(m_mutex);
  CheckState();
  const auto orderId = TakeOrderId();
  m_client->placeOrder(orderId, contract, order);
  UpdateLastRequestTime();
  m_condition.notify_all();

  return orderId;
}

void Client::CancelOrder(trdk::OrderId id) {
  const Lock lock(m_mutex);
  CheckState();
  AssertLe(0, id);
  m_client->cancelOrder(::OrderId(id));
  UpdateLastRequestTime();
  m_condition.notify_all();
}

void Client::LogConnectionAttempt() const throw() {
  m_ts.GetTsLog().Debug("Connecting to \"%1%:%2%\" with client ID %3%...",
                        m_host, m_port, m_clientId);
}
void Client::LogConnect() const throw() {
  m_ts.GetTsLog().Info("Connected to \"%1%:%2%\" with client ID %3%.", m_host,
                       m_port, m_clientId);
}

void Client::LogDisconnectAttempt() const throw() {
  m_ts.GetTsLog().Debug("Disconnecting from \"%1%:%2%\" with client ID %3%...",
                        m_host, m_port, m_clientId);
}

void Client::LogError(const int id, const int code, const IBString &message) {
  switch (code) {
    // case 200: // No security definition has been found for the request.
    case 201:  // Order rejected - Reason:
      m_ts.GetTsLog().Error("Order rejected: %1%.", message);
      break;
    case 202:  // Order canceled - Reason:
      /*
      m_log.Debug(
              INTERACTIVE_BROKERS_CLIENT_CONNECTION_NAME
                      " order canceled: %1%.",
              message);
      */
      break;
    // case 203: //  The security <security> is not available or allowed
    //  for this account.
    case 399: {
      std::string messageCopy(message);
      std::for_each(messageCopy.begin(), messageCopy.end(), [](char &ch) {
        if (ch == '\n') {
          ch = ' ';
        }
      });
      m_ts.GetTsLog().Error("Order (ID %1%) error: \"%2%\".", id, messageCopy);
    } break;
    case 502:  // Couldn't connect to TWS.
      m_ts.GetTsLog().Error("Couldn't connect to TWS: \"%1%\".", message);
      break;
    case 1100:  // Connectivity between IB and TWS has been lost.
      m_ts.GetTsLog().Warn(message.c_str());
      break;
    // case 2100: // New account data requested from TWS. API client has
    // been unsubscribed from account data.
    // case 2101: // Unable to subscribe to account as the following
    // clients are subscribed to a different account.
    // case 2102: // Unable to modify this order as it is still being
    // processed.
    // case 2103: // A market data farm is disconnected.
    case 2104:  // A market data farm is connected.
    case 2106:  // A historical data farm is connected.
      m_ts.GetMdsLog().Debug(
          "\"%1%\" (error code: %2%, order or ticket ID: %3%).", message, code,
          id);
      break;
    case 2105:  // A historical data farm is disconnected.
    case 2107:  // A historical data farm connection has become inactive
                // but should be available upon demand.
    case 2108:  // A market data farm connection has become inactive but
      // should be available upon demand.
      m_ts.GetMdsLog().Warn(
          "\"%1%\" (error code: %2%, order or ticket ID: %3%).", message, code,
          id);
      break;
    case 2109:  // Order Event Warning: Attribute “Outside Regular Trading
                // Hours” is ignored based on the order type and
                // destination. PlaceOrder is now processed.
    case 2110:  // Connectivity between TWS and server is broken. It will
      // be restored automatically.
      m_ts.GetTsLog().Warn(
          "\"%1%\" (error code: %2%, order or ticket ID: %3%).", message, code,
          id);
      break;
    default:
      m_ts.GetTsLog().Error(
          "\"%1%\" (error code: %2%, order or ticket ID: %3%).", message, code,
          id);
      break;
  }
}

Client::OrderStatusesMap Client::GetOrderStatusesMap() {
  OrderStatusesMap r;
  auto mp = [](const char *f,
               const OrderStatus &s) -> OrderStatusesMap::value_type {
    return std::make_pair(std::string(f), s);
  };
  r.insert(mp("PendingSubmit", ORDER_STATUS_SENT));
  r.insert(mp("PendingCancel", ORDER_STATUS_SENT));
  r.insert(mp("PreSubmitted", ORDER_STATUS_SENT));
  r.insert(mp("Submitted", ORDER_STATUS_SUBMITTED));
  r.insert(mp("Cancelled", ORDER_STATUS_CANCELLED));
  r.insert(mp("ApiCancelled", ORDER_STATUS_CANCELLED));
  r.insert(mp("Filled", ORDER_STATUS_FILLED));
  r.insert(mp("Inactive", ORDER_STATUS_ERROR));
  return r;
}

void Client::UpdateNextPingRequestTime() {
  m_nextPingTime = boost::get_system_time() + pingRequestPeriod;
  m_timeoutTime = pt::not_a_date_time;
}

void Client::UpdateLastRequestTime() {
  if (m_timeoutTime != pt::not_a_date_time) {
    return;
  }
  m_timeoutTime = boost::get_system_time() + timeout;
}

void Client::UpdateLastResponseTime() { UpdateNextPingRequestTime(); }

void Client::RequestCurrentTime() {
  m_state = PING_STATE_ACK;
  m_client->reqCurrentTime();
  UpdateLastRequestTime();
}

::OrderId Client::TakeOrderId() {
  AssertLe(0, m_seqNumber);
  return m_seqNumber++;
}

TickerId Client::TakeTickerId() {
  Assert(m_seqNumber >= 0);
  return m_seqNumber++;
}

void Client::HandleError(const int id,
                         const int code,
                         const IBString & /*message*/) {
  switch (code) {
    case 162:  // Historical market data Service error message.
    case 321:  // Server error when validating an API client request.
    {
      const SecurityRequest *const request = GetHistoryRequest(id);
      if (request
          // sometimes TWS sends it two or more times:
          && !IsSubscribed(m_marketDataRequests, *request->security)) {
        //! @todo Check data type.
        SendMarketDataRequest(*request->security);
      }
    } break;
    case 103:  // Duplicate order ID.
    case 110:  // The price does not conform to the minimum price
               // variation for this contract.
    case 201:  // Order rejected - Reason:
    default:
      m_orderStatusSignal(id, 0, ORDER_STATUS_ERROR, 0, 0, .0, m_callBackList);
      break;
    case 202:  // Order cancelled - Reason:
      m_orderStatusSignal(id, 0, ORDER_STATUS_CANCELLED, 0, 0, .0,
                          m_callBackList);
      break;
    case 1300:  // TWS socket port has been reset and this connection is
      // being dropped.
      AssertEq(-1, id);
      m_connectionState = CONNECTION_STATE_NOT_CONNECTED;
      break;
  }
}

void Client::CheckState() const {
  if (m_seqNumber < 0) {
    m_ts.GetTsLog().Error("No seqnumber specified.");
  }
  if (m_seqNumber < 0 || !m_connectionState) {
    throw TradingSystem::ConnectionDoesntExistError("Connection doesn't exist");
  }
}

void Client::CheckTimeout() const {
  if (m_timeoutTime != pt::not_a_date_time && m_timeoutTime <= m_clientNow) {
    m_ts.GetTsLog().Error("Connection TIMEOUT!");
    // m_isConnected = false; - trying to work
    return;
  }
  switch (m_state) {
    case PING_STATE_IDLE:
      Assert(m_nextPingTime != pt::not_a_date_time);
      if (m_nextPingTime > m_clientNow) {
        break;
      }
    /* no break! */
    case PING_STATE_REQ:
      const_cast<Client *>(this)->RequestCurrentTime();
      break;
    default:
      Assert(m_state == PING_STATE_ACK);
      break;
  }
}

ib::Security *Client::GetMarketDataRequest(const TickerId &tickerId) {
  const auto &index = m_marketDataRequests.get<ByTicker>();
  const auto &pos = index.find(tickerId);
  if (pos == index.end()) {
    m_ts.GetMdsLog().Warn("Couldn't find Market Data Request for ticker %1%.",
                          tickerId);
    return nullptr;
  }
  return &*pos->security;
}

const Client::SecurityHistoryRequest *Client::GetHistoryRequest(
    const TickerId &tickerId) {
  const auto &index = m_historyRequest.get<ByTicker>();
  const auto &pos = index.find(tickerId);
  if (pos == index.end()) {
    m_ts.GetMdsLog().Warn("Couldn't find History Request for ticker %1%.",
                          tickerId);
    return nullptr;
  }
  return &*pos;
}

ib::Security *Client::GetBarsRequest(const TickerId &tickerId) {
  const auto &index = m_barsRequest.get<ByTicker>();
  const auto &pos = index.find(tickerId);
  if (pos == index.end()) {
    m_ts.GetMdsLog().Warn("Couldn't find Bars Request for ticker %1%.",
                          tickerId);
    return nullptr;
  }
  return &*pos->security;
}

bool Client::IsSubscribed(const SecurityRequestList &list,
                          const ib::Security &security) {
  const auto &index = list.get<BySecurity>();
  return index.find(const_cast<ib::Security *>(&security)) != index.end();
}

bool Client::IsSubscribed(const SecurityRequestList &requested,
                          const PostponedSecurityRequestList &postponed,
                          const ib::Security &security) {
  return IsSubscribed(requested, security) ||
         std::find(postponed.begin(), postponed.end(), &security) !=
             postponed.end();
}

///////////////////////////////////////////////////////////////////////////////

void Client::commissionReport(const CommissionReport &) {}

void Client::orderStatus(::OrderId id,
                         const IBString &statusText,
                         int filled,
                         int remaining,
                         double /*avgFillPrice*/,
                         int permId,
                         int parentId,
                         double lastFillPrice,
                         int /*clientId*/,
                         const IBString & /*whyHeld*/) {
  /*
          m_log.Debug(
                  "Order status: "
                          " ID: %1%"
                          ", status: %2%"
                          ", filled: %3%"
                          ", remaining: %4%"
                          ", lastFillPrice: %5%"
                          ", whyHeld: \"%6%\".",
                  id,
                  statusText,
                  filled,
                  remaining,
                  lastFillPrice,
                  whyHeld);
  */

  const OrderStatusesMap::const_iterator statusPos =
      m_orderStatusesMap.find(statusText);
  Assert(statusPos != m_orderStatusesMap.end());
  if (statusPos == m_orderStatusesMap.end()) {
    m_ts.GetTsLog().Error(
        "Failed to decode status order"
        " (ID: %1%, status: \"%2%\", parent ID: %3%).",
        id, statusText, parentId);
    return;
  }
  Assert(m_seqNumber < 0 || m_seqNumber > id);
  if (statusPos->second == ORDER_STATUS_ERROR) {
    m_ts.GetTsLog().Error(
        "Order %1% has been accepted by the system (simulated orders)"
        " or an exchange (native orders) but that currently the order"
        " is inactive due to system, exchange or other issues."
        " Trading system order status: \"%2%\".",
        id, statusText);
  }
  m_orderStatusSignal(id, permId, statusPos->second, filled, remaining,
                      lastFillPrice, m_callBackList);
}

void Client::currentTime(long time) {
  AssertEq(PING_STATE_ACK, m_state);
  if (m_state != PING_STATE_ACK) {
    m_ts.GetTsLog().Warn("Server current time arrived without request: %1%.",
                         pt::from_time_t(time));
    return;
  }
  m_ts.GetTsLog().Info("Server current time: %1%.", pt::from_time_t(time));
  UpdateNextPingRequestTime();
  m_state = PING_STATE_IDLE;
}

void Client::error(const int id, const int code, const IBString message) {
  LogError(id, code, message);
  HandleError(id, code, message);
}

void Client::tickPrice(TickerId tickerId,
                       TickType field,
                       double price,
                       int /*canAutoExecute*/) {
  if (price < 0) {
    return;
  }
  Level1TickValue (*valueCtor)(const ScaledPrice &);
  switch (field) {
    default:
      return;
    case LAST:
      valueCtor = &Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>;
      break;
    case BID:
      valueCtor = Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>;
      break;
    case ASK:
      valueCtor = Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>;
      break;
  }
  const auto &timeMeasurement =
      m_ts.GetContext().StartStrategyTimeMeasurement();
  const auto &now = m_ts.GetContext().GetCurrentTime();
  ib::Security *const security = GetMarketDataRequest(tickerId);
  if (!security) {
    return;
  }
  if (m_securityInSwitching && m_securityInSwitching == security) {
    {
      boost::mutex::scoped_lock lock(m_switchMutex);
      Assert(m_securityInSwitching);
      Assert(m_securityInSwitching == security);
      m_securityInSwitching = nullptr;
    }
    m_switchCondition.notify_all();
  }
  security->AddLevel1Tick(now, valueCtor(security->ScalePrice(price)),
                          timeMeasurement);
}

void Client::tickSize(TickerId tickerId, TickType field, int size) {
  if (!size) {
    return;
  }
  AssertLt(0, size);
  Level1TickValue (*valueCtor)(const Qty &);
  switch (field) {
    default:
      return;
    case VOLUME:
      valueCtor = Level1TickValue::Create<LEVEL1_TICK_TRADING_VOLUME>;
      break;
    case LAST_SIZE:
      valueCtor = Level1TickValue::Create<LEVEL1_TICK_LAST_QTY>;
      break;
    case BID_SIZE:
      valueCtor = Level1TickValue::Create<LEVEL1_TICK_BID_QTY>;
      break;
    case ASK_SIZE:
      valueCtor = Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>;
      break;
  }
  const auto &timeMeasurement =
      m_ts.GetContext().StartStrategyTimeMeasurement();
  const auto &now = m_ts.GetContext().GetCurrentTime();
  ib::Security *const security = GetMarketDataRequest(tickerId);
  if (!security) {
    return;
  }
  if (m_securityInSwitching && m_securityInSwitching == security) {
    {
      boost::mutex::scoped_lock lock(m_switchMutex);
      Assert(m_securityInSwitching);
      Assert(m_securityInSwitching == security);
      m_securityInSwitching = nullptr;
    }
    m_switchCondition.notify_all();
  }
  security->AddLevel1Tick(now, valueCtor(size), timeMeasurement);
}

void Client::tickOptionComputation(TickerId /*tickerId*/,
                                   TickType /*tickType*/,
                                   double /*impliedVol*/,
                                   double /*delta*/,
                                   double /*optPrice*/,
                                   double /*pvDividend*/,
                                   double /*gamma*/,
                                   double /*vega*/,
                                   double /*theta*/,
                                   double /*undPrice*/) {}

void Client::tickGeneric(TickerId /*tickerId*/,
                         TickType /*tickType*/,
                         double /*value*/) {}

void Client::tickString(TickerId /*tickerId*/,
                        TickType /*tickType*/,
                        const IBString & /*value*/) {}

void Client::tickEFP(TickerId /*tickerId*/,
                     TickType /*tickType*/,
                     double /*basisPoints*/,
                     const IBString & /*formattedBasisPoints*/,
                     double /*totalDividends*/,
                     int /*holdDays*/,
                     const IBString & /*futureExpiry*/,
                     double /*dividendImpact*/,
                     double /*dividendsToExpiry*/) {}

void Client::openOrder(::OrderId /*orderId*/,
                       const Contract &,
                       const Order &,
                       const OrderState &) {}

void Client::openOrderEnd() {}

void Client::winError(const IBString &message, int code) {
  m_ts.GetTsLog().Error(
      "Error occurred on client side: \"%1%\" (error code: %2%).", message,
      code);
}

void Client::connectionClosed() {}

void Client::updateAccountValue(const IBString &key,
                                const IBString &val,
                                const IBString & /*currency*/,
                                const IBString &accountName) {
  Assert(!boost::math::isnan(m_accountInfo));
  if (!m_accountInfo) {
    m_ts.GetTsLog().Error("Received account info but not requested.");
    return;
  } else if (!m_account.empty() && !boost::iequals(m_account, accountName)) {
    return;
  }

  bool isExcessLiquiditySource = false;
  if (key == "CashBalance") {
    m_accountInfo->cashBalance = boost::lexical_cast<double>(val);
  } else if (key == "EquityWithLoanValue") {
    m_accountInfo->equityWithLoanValue = boost::lexical_cast<double>(val);
    isExcessLiquiditySource = true;
  } else if (key == "MaintMarginReq") {
    m_accountInfo->maintenanceMargin = boost::lexical_cast<double>(val);
    isExcessLiquiditySource = true;
  }

  if (isExcessLiquiditySource) {
    const auto excessLiquidity =
        m_accountInfo->equityWithLoanValue - m_accountInfo->maintenanceMargin;
    m_accountInfo->excessLiquidity = excessLiquidity;
  }
}

void Client::updatePortfolio(const Contract & /*contract*/,
                             int /*position*/,
                             double /*marketPrice*/,
                             double /*marketValue*/,
                             double /*averageCost*/,
                             double /*unrealizedPnl*/,
                             double /*realizedPnl*/,
                             const IBString & /*accountName*/) {}

void Client::updateAccountTime(const IBString & /*timeStamp*/) {}

void Client::accountDownloadEnd(const IBString & /*accountName*/) {}

void Client::nextValidId(::OrderId id) {
  Assert(m_seqNumber < 0);
  const auto prevVal = m_seqNumber;
  m_seqNumber = id;
  if (prevVal != -1) {
    m_ts.GetTsLog().Debug("Next order ID: %1% -> %2%.", prevVal, m_seqNumber);
  } else {
    m_ts.GetTsLog().Debug("Next order ID: %1%.", m_seqNumber);
  }
  Assert(m_seqNumber >= 0);
}

void Client::contractDetails(int /*reqId*/,
                             const ContractDetails & /*contractDetails*/) {}

void Client::bondContractDetails(int /*reqId*/,
                                 const ContractDetails & /*contractDetails*/) {}

void Client::contractDetailsEnd(int /*reqId*/) {}

void Client::execDetails(int /*reqId*/,
                         const Contract & /*contract*/,
                         const Execution & /*execution*/) {}

void Client::execDetailsEnd(int /*reqId*/) {}

void Client::updateMktDepth(TickerId /*tickerId*/,
                            int /*position*/,
                            int /*operation*/,
                            int /*side*/,
                            double /*price*/,
                            int /*size*/) {}

void Client::updateMktDepthL2(TickerId tickerId,
                              int /*position*/,
                              IBString marketMaker,
                              int operation,
                              int side,
                              double /*price*/,
                              int /*size*/) {
  AssertFail("Market Depth Level II not yet supported by security.");
  const auto &requestsIndex = m_marketDepthLevel2Requests.get<ByTicker>();
  const auto i = requestsIndex.find(tickerId);
  if (i == requestsIndex.end()) {
    m_ts.GetMdsLog().Debug(
        "Couldn't find Market Depth Data Request for ticker %1%.", tickerId);
    return;
  }
  bool isAsk = false;
  switch (side) {
    case 0:
      isAsk = true;
      break;
    case 1:
      isAsk = false;
      break;
    default:
      AssertFail("Unknown side.");
      return;
  }
  switch (operation) {
    case 0:  // insert (insert this new order into the row identified by
             // 'position')·
    case 1:  // update (update the existing order in the row identified by
             // 'position')·
      // i->second->UpdateLevel2IbLine(now, position, isAsk, price, size);
      break;
    case 2:  // delete (delete the existing order at the row identified by
             // 'position')
      // i->second->DeleteLevel2IbLine(now, position, isAsk, price, size);
      break;
    default:
      AssertFail("Unknown operation.");
      return;
  }
}

void Client::updateNewsBulletin(int /*msgId*/,
                                int /*msgType*/,
                                const IBString & /*newsMessage*/,
                                const IBString & /*originExch*/) {}

void Client::managedAccounts(const IBString & /*accountsList*/) {}

void Client::receiveFA(faDataType /*pFaDataType*/, const IBString & /*cxml*/) {}

namespace {

pt::ptime ParseHistoryPointTime(const std::string &source,
                                MarketDataSource::Log &log,
                                bool isFinished) {
  boost::smatch match;
  const boost::regex regex(
      !isFinished ? "(\\d{4})(\\d{2})(\\d{2}) +(\\d{2}):(\\d{2}):(\\d{2})"
                  : "finished-\\d{4}\\d{2}\\d{2} +\\d{2}:\\d{2}:\\d{2}"
                    "-(\\d{4})(\\d{2})(\\d{2}) +(\\d{2}):(\\d{2}):(\\d{2})");
  if (!boost::regex_match(source, match, regex)) {
    log.Error("Failed to extract history point date time from \"%1%\".",
              source);
    throw Exception("Failed to extract history point date time");
  }

  try {
    return pt::ptime(
        pt::ptime::date_type(boost::lexical_cast<unsigned short>(match.str(1)),
                             boost::lexical_cast<unsigned short>(match.str(2)),
                             boost::lexical_cast<unsigned short>(match.str(3))),
        pt::ptime::time_duration_type(
            boost::lexical_cast<unsigned short>(match.str(4)),
            boost::lexical_cast<unsigned short>(match.str(5)),
            boost::lexical_cast<unsigned short>(match.str(6))));
  } catch (const boost::bad_lexical_cast &) {
    log.Error("Failed to extract history point date time from \"%1%\".",
              source);
    throw Exception("Failed to extract history point date time");
  }
}
}

void Client::historicalData(TickerId tickerId,
                            const IBString &timeStr,
                            double openPrice,
                            double highPrice,
                            double lowPrice,
                            double closePrice,
                            int volume,
                            int /*barCount*/,
                            double /*WAP*/,
                            int /*hasGaps*/) {
  const SecurityHistoryRequest *const request = GetHistoryRequest(tickerId);
  if (!request) {
    return;
  }
  Assert(request->security);
  Assert(request->security->IsLevel1Required() ||
         request->security->IsBarsRequired() ||
         (request->security->IsTestSource() &&
          request->security->IsTradesRequired()));

  const bool isFinished = boost::starts_with(timeStr, "f");
  Assert(!isFinished || boost::starts_with(timeStr, "finished-"));
  Assert(!isFinished || (IsEqual(lowPrice, -1.0) && IsEqual(highPrice, -1.0)));

  if (!isFinished) {
    const auto &time =
        ParseHistoryPointTime(timeStr, m_ts.GetMdsLog(), isFinished);
    const bool isRequiredTime =
        request->subRequestStart <= time && time <= request->subRequestEnd;

    if (isRequiredTime && request->security->IsLevel1Required()) {
      const auto &prevExpiration = std::prev(request->expiration);
      if (!prevExpiration || pt::ptime(prevExpiration->GetDate()) < time) {
        Assert(m_historyUpdates.find(request->security) ==
                   m_historyUpdates.cend() ||
               !m_historyUpdates[request->security].empty());
#ifdef BOOST_DISABLE_ASSERTS
        if (m_historyUpdates.find(request->security) !=
            m_historyUpdates.cend()) {
          AssertLt(m_historyUpdates[request->security].back().time, time);
        }
#endif
        m_historyUpdates[request->security].emplace_back(
            HistoryUpdate{time, openPrice, highPrice, lowPrice, closePrice,
                          request->expiration});
      }
    }

    if (isRequiredTime && request->security->IsBarsRequired()) {
      AssertFail("History bars is not supported anymore");
      ib::Security::Bar bar(time, ib::Security::Bar::TRADES);
      bar.openPrice = request->security->ScalePrice(openPrice);
      bar.highPrice = request->security->ScalePrice(highPrice);
      bar.lowPrice = request->security->ScalePrice(lowPrice);
      bar.closePrice = request->security->ScalePrice(closePrice);
      bar.volume = volume;
      //! See request for detail about frame size:
      bar.period = pt::seconds(request->security->IsLevel1Required() ? 1 : 5);
      request->security->AddBar(std::move(bar));
    }

    return;
  }

  Assert(request->expiration);

  const auto &prevExpiration = std::prev(request->expiration);
  if (prevExpiration) {
    if (request->subRequestStart <= pt::ptime(prevExpiration->GetDate())) {
      AssertEq(pt::ptime(prevExpiration->GetDate()), request->subRequestStart);
      m_ts.GetMdsLog().Debug(
          "Finished Level I"
          " market data history contract request"
          " for \"%1%\" (%2%%3%, expiration: %4%"
          ", ticker ID: %5%).",
          *request->security, request->security->GetSymbol().GetSymbol(),
          request->expiration->GetContract(true),
          request->expiration->GetDate(), request->tickerId);
    }
    if (SendMarketDataHistoryRequest(
            *request->security, request->requestsSequenceStartTime,
            request->subRequestStart, request->numberOfPrevRequests, .0)) {
      return;
    }
  }

  m_ts.GetMdsLog().Debug(
      "Finished Level I market data history request for \"%1%\""
      " (ticker ID: %2%).",
      *request->security, request->tickerId);
  FlushHistory(*request->security);
  SendMarketDataRequest(*request->security);
}

namespace {

template <typename HistoryUpdate>
void FlushHistoryUpdate(const HistoryUpdate &raw,
                        const HistoryUpdate &adjusted,
                        double adjustRatio,
                        ib::Security &security,
                        const ContractExpiration &expiration,
                        std::ostream &log,
                        Context &context) {
  AssertEq(raw.time, adjusted.time);
  Assert(IsEqual(raw.openPrice * adjustRatio, adjusted.openPrice));
  Assert(IsEqual(raw.highPrice * adjustRatio, adjusted.highPrice));
  Assert(IsEqual(raw.lowPrice * adjustRatio, adjusted.lowPrice));
  Assert(IsEqual(raw.closePrice * adjustRatio, adjusted.closePrice));

  log << raw.time << ',' << expiration.GetContract(true) << ','
      << expiration.GetDate() << ',' << adjustRatio << ',' << raw.openPrice
      << ',' << adjusted.openPrice << ',' << raw.closePrice << ','
      << adjusted.closePrice << ',' << raw.highPrice << ','
      << adjusted.highPrice << ',' << raw.lowPrice << ',' << adjusted.lowPrice
      << std::endl;

  security.AddLevel1Tick(raw.time,
                         Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(
                             security.ScalePrice(adjusted.openPrice)),
                         Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(
                             security.ScalePrice(adjusted.highPrice)),
                         Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(
                             security.ScalePrice(adjusted.lowPrice)),
                         Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(
                             security.ScalePrice(adjusted.closePrice)),
                         context.StartStrategyTimeMeasurement());
}
}

void Client::FlushHistory(ib::Security &security) {
  const auto &securityHistoyIt = m_historyUpdates.find(&security);
  if (securityHistoyIt == m_historyUpdates.cend()) {
    return;
  }
  auto &securityHistory = securityHistoyIt->second;
  Assert(!securityHistory.empty());

  const auto &now = m_ts.GetContext().GetCurrentTime();

  fs::path logPath =
      m_ts.GetContext().GetSettings().GetLogsInstanceDir() / "History";
  logPath /= SymbolToFileName(
      (boost::format("%1%_%2%%3%%4%_%5%%6%%7%") % security.GetSymbol() %
       now.date().year() % now.date().month().as_number() % now.date().day() %
       now.time_of_day().hours() % now.time_of_day().minutes() %
       now.time_of_day().seconds())
          .str(),
      "csv");
  m_ts.GetMdsLog().Debug("Flushing %1% records history records (%2%)...",
                         securityHistory.size(), logPath);

  const bool isNew = !fs::exists(logPath);
  if (isNew) {
    fs::create_directories(logPath.branch_path());
  }
  std::ofstream log(logPath.string(),
                    std::ios::out | std::ios::ate | std::ios::app);
  if (!log) {
    m_ts.GetMdsLog().Error("Failed to open log file %1%", logPath);
    throw trdk::MarketDataSource::Error("Failed to open log file");
  }
  log << "Time"
      << ",Contact"
      << ",Expiration"
      << ",Adj. ratio"
      << ",Open (raw),Open (adj)"
      << ",Close (raw),Close (adj)"
      << ",High (raw),High (adj)"
      << ",Low (raw),Low (adj)" << std::endl;

  std::sort(securityHistory.begin(), securityHistory.end());

  auto startB = securityHistory.cbegin();
  // https://www.interactivebrokers.com/en/software/tws/usersguidebook/technicalanalytics/continuous.htm
  for (auto i = startB; i != securityHistory.cend(); ++i) {
    if (startB->expiration == i->expiration) {
      continue;
    }

    const auto closeB = std::prev(i);
    auto closeA = i;
    for (;; ++closeA) {
      const auto &nextCloseA = std::next(closeA);
      if (nextCloseA == securityHistory.cend() ||
          nextCloseA->expiration != closeA->expiration) {
        break;
      }
    }

    // To generate a normalized historical data series for the above
    // sequence, call CL MAY'15 as Contract A and CL APR'15 as Contract B.
    // Then follow the steps below:
    // 1. Take the closing price of Contract B and Contract A on 20150317.
    // 2. Calculate the ratio of
    //    closingPrice(Contract A) / closingPrice(Contract B).
    const auto ratio = closeA->closePrice / closeB->closePrice;

    // Multiply all Contract B data for 20150317 and all prior dates
    // by this ratio. This results in the adjusted time series of Contract B
    // having the SAME closing price on 20150317 as Contract A.
    for (; startB != i; ++startB) {
      auto adjusted = *startB;
      adjusted.openPrice *= ratio;
      adjusted.highPrice *= ratio;
      adjusted.lowPrice *= ratio;
      adjusted.closePrice *= ratio;
      FlushHistoryUpdate(*startB, adjusted, ratio, security,
                         *startB->expiration, log, m_ts.GetContext());
    }
  }

  // Actual data.
  for (auto i = startB; i != securityHistory.cend(); ++i) {
    FlushHistoryUpdate(*i, *i, 1.0, security, *i->expiration, log,
                       m_ts.GetContext());
  }

  m_historyUpdates.erase(securityHistoyIt);

  m_ts.GetMdsLog().Debug("History flushing completed.");
}

void Client::scannerParameters(const IBString & /*xml*/) {}

void Client::scannerData(int /*reqId*/,
                         int /*rank*/,
                         const ContractDetails & /*contractDetails*/,
                         const IBString & /*distance*/,
                         const IBString & /*benchmark*/,
                         const IBString & /*projection*/,
                         const IBString & /*legsStr*/) {}

void Client::scannerDataEnd(int /*reqId*/) {}

void Client::realtimeBar(TickerId /*tickerId*/,
                         long /*time*/,
                         double /*openPrice*/,
                         double /*highPrice*/,
                         double /*lowPrice*/,
                         double /*closePrice*/,
                         long /*volume*/,
                         double /*wap*/,
                         int /*count*/) {
  AssertFail("Received real time bar from IB.");
  /*
  auto *const security = GetBarsRequest(tickerId);
  if (!security) {
          return;
  }
  Assert(security->IsBarsRequired());
  ib::Security::Bar bar(
          pt::from_time_t(time),
          // Currently only 5 second bars are supported, if any other value is
          // used, an exception will be thrown:
          pt::seconds(5),
          ib::Security::Bar::TRADES);
  bar.openPrice = security->ScalePrice(openPrice);
  bar.highPrice = security->ScalePrice(highPrice);
  bar.lowPrice = security->ScalePrice(lowPrice);
  bar.closePrice = security->ScalePrice(closePrice);
  bar.volume = volume;
  bar.count = count;
  security->AddBar(bar);
  */
}

void Client::fundamentalData(TickerId /*reqId*/, const IBString & /*data*/) {}

void Client::deltaNeutralValidation(int /*reqId*/,
                                    const UnderComp & /*underComp*/) {}

void Client::tickSnapshotEnd(int /*reqId*/) {}

void Client::marketDataType(TickerId /*reqId*/, int /*marketDataType*/) {}

void Client::position(const IBString &account,
                      const Contract &contract,
                      int size,
                      double avgCost) {
  const bool isInitial = m_connectionState < CONNECTION_STATE_READY;

  m_ts.GetTsLog().Info(
      "Position info for account \"%1%\""
      " - \"%2%:%3% (%4%)\" = %5% (average cost: %6%).",
      account, contract.symbol, contract.currency, contract.secType, size,
      avgCost);

  //! @todo place for optimization (if position will be used not only at
  //! start):
  foreach (auto &security, m_ts.m_securities) {
    //! @todo compares only symbols, not exchanges
    if (security->GetSymbol().GetSymbol() == contract.symbol) {
      security->SetBrokerPosition(size, isInitial);
      break;
    }
  }

  if (m_ts.m_positions) {
    Symbol symbol;

    if (contract.secType == "STK") {
      symbol = Symbol(
          (boost::format("%1%/%2%::STK") % contract.symbol % contract.currency)
              .str());
    } else {
      boost::format message(
          "Security type \"%1%\" is not supported by position list");
      message % contract.secType;
      throw MethodDoesNotImplementedError(message.str().c_str());
    }

    if (symbol) {
      ib::TradingSystem::Position position(account, symbol, size);

      auto &index = m_ts.m_positions->get<ib::TradingSystem::BySymbol>();

      const ib::TradingSystem::PositionsWriteLock positionsLock(
          m_ts.m_positionsMutex);

      auto it = index.find(boost::make_tuple(
          boost::cref(account), boost::cref(position.symbol.GetCurrency()),
          boost::cref(position.symbol.GetSymbol())));
      if (it == index.end()) {
        index.insert(position);
      } else {
        index.modify(it, [&position](ib::TradingSystem::Position &storage) {
          storage = position;
        });
      }
    }
  }
}

void Client::positionEnd() {
  AssertGt(CONNECTION_STATE_READY, m_connectionState);
  m_ts.GetTsLog().Debug("Positions info completed.");
  // m_client->cancelPositions();
  m_connectionState = CONNECTION_STATE_READY;
  FlushPostponedMarketDataSubscription();
}

void Client::accountSummary(int,
                            const IBString &,
                            const IBString &,
                            const IBString &,
                            const IBString &) {}

void Client::accountSummaryEnd(int) {}

void Client::verifyMessageAPI(const IBString &apiData) {
  m_ts.GetTsLog().Error("API data: \"%1%\".", apiData);
}

void Client::verifyCompleted(bool isSuccessful, const IBString &errorText) {
  m_ts.GetTsLog().Error("Is successful: %1%; Error: \"%2%\".", isSuccessful,
                        errorText);
}

void Client::displayGroupList(int /*reqId*/, const IBString & /*groups*/) {
  AssertFail("Unexpected method call.");
}

void Client::displayGroupUpdated(int /*reqId*/,
                                 const IBString & /*contractInfo*/) {
  AssertFail("Unexpected method call.");
}

void Client::SwitchToNextContract(ib::Security &security) {
  boost::mutex::scoped_lock switchLock(m_switchMutex);
  Assert(!m_securityInSwitching);

  auto &index = m_marketDataRequests.get<BySecurity>();
  auto oldRequest = index.find(&security);
  if (oldRequest == index.end()) {
    throw MarketDataSource::Error("Failed to find old request");
  }

  const auto prevExpiration = m_ts.GetContext().GetExpirationCalendar().Find(
      security.GetSymbol(), m_ts.GetContext().GetCurrentTime().date());
  if (!prevExpiration) {
    boost::format error("Failed to find current expiration info for \"%1%\"");
    error % security;
    throw trdk::MarketDataSource::Error(error.str().c_str());
  }
  AssertEq(security.GetExpiration().GetDate(), prevExpiration->GetDate());
  auto nextExpiration = prevExpiration;
  if (!++nextExpiration) {
    boost::format error("Failed to find next expiration info for \"%1%\"");
    error % security;
    throw trdk::MarketDataSource::Error(error.str().c_str());
  }

  {
    Lock lock(m_mutex);

    m_ts.GetMdsLog().Info(
        "Switching %1% contract from %2% (%3%) to %4% (%5%)...", security,
        FormatLocalFuturesSymbol(security.GetSymbol().GetSymbol(),
                                 *prevExpiration),
        prevExpiration->GetDate(),
        FormatLocalFuturesSymbol(security.GetSymbol().GetSymbol(),
                                 *nextExpiration),
        nextExpiration->GetDate());

    m_client->cancelMktData(oldRequest->tickerId);
    m_marketDataRequests.erase(oldRequest);

    m_securityInSwitching = &security;

    security.SetExpiration(pt::not_a_date_time, *nextExpiration);
    SendMarketDataRequest(security);
  }

  m_switchCondition.wait(switchLock);

  Assert(!m_securityInSwitching);

  m_ts.GetMdsLog().Info(
      "%1% switched to next contract %4% (%5%) from %2% (%3%).", security,
      FormatLocalFuturesSymbol(security.GetSymbol().GetSymbol(),
                               *prevExpiration),
      prevExpiration->GetDate(),
      FormatLocalFuturesSymbol(security.GetSymbol().GetSymbol(),
                               *nextExpiration),
      nextExpiration->GetDate());
}

///////////////////////////////////////////////////////////////////////////////
