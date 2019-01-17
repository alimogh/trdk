/*******************************************************************************
 *   Created: 2017/10/26 14:44:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MarketDataConnection.hpp"
#include "Product.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Crypto;
using namespace TradingLib;
using namespace Interaction;
using namespace Rest;
using namespace Coinbase;

namespace pc = Poco;
namespace net = pc::Net;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

namespace {

struct Settings : Rest::Settings {
  std::string apiKey;
  std::vector<unsigned char> apiSecret;
  std::string apiPassphrase;

  explicit Settings(const ptr::ptree& conf, ModuleEventsLog& log)
      : Rest::Settings(conf, log),
        apiKey(conf.get<std::string>("config.auth.apiKey")),
        apiSecret(
            Base64::Decode(conf.get<std::string>("config.auth.apiSecret"))),
        apiPassphrase(conf.get<std::string>("config.auth.apiPassphrase")) {
    log.Info("API key: \"%1%\". API secret: %2%. API pass phrase: %3%.",
             apiKey,                                         // 1
             apiSecret.empty() ? "not set" : "is set",       // 2
             apiPassphrase.empty() ? "not set" : "is set");  // 3
  }
};

template <Level1TickType priceType, Level1TickType qtyType>
std::map<Price, std::pair<Level1TickValue, Level1TickValue>> ReadBook(
    const ptr::ptree& source) {
  std::map<Price, std::pair<Level1TickValue, Level1TickValue>> result;
  for (const auto& lines : source) {
    boost::optional<Price> price;
    for (const auto& val : lines.second) {
      if (!price) {
        price.emplace(val.second.get_value<Price>());
      } else {
        const auto& qty = val.second.get_value<Qty>();
        auto it = result.emplace(
            *price, std::make_pair(Level1TickValue::Create<priceType>(*price),
                                   Level1TickValue::Create<qtyType>(qty)));
        if (!it.second) {
          it.first->second.second = Level1TickValue::Create<qtyType>(
              it.first->second.second.GetValue() + qty);
        }
      }
    }
  }
  return result;
}

Volume ParseFee(const ptr::ptree& order) {
  return order.get<Volume>("fill_fees");
}
}  // namespace

////////////////////////////////////////////////////////////////////////////////

namespace {

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

  explicit Request(const std::string& request,
                   const std::string& method,
                   const std::string& uriParams,
                   const Context& context,
                   ModuleEventsLog& log,
                   ModuleTradingLog* tradingLog = nullptr)
      : Base("/" + request,
             request,
             method,
             uriParams,
             context,
             log,
             tradingLog) {}

  Request(Request&&) = default;
  Request(const Request&) = delete;
  Request& operator=(Request&&) = delete;
  Request& operator=(const Request&) = delete;
  ~Request() override = default;

 protected:
  void CheckErrorResponse(const net::HTTPResponse& response,
                          const std::string& responseContent,
                          size_t attemptNumber) const override {
    try {
      const auto& message =
          ReadJson(responseContent).get<std::string>("message");
      if (boost::iequals(message, "Order already done") ||
          boost::iequals(message, "order not found")) {
        throw OrderIsUnknownException(message.c_str());
      }
      if (boost::iequals(message, "Insufficient funds")) {
        throw InsufficientFundsException(message.c_str());
      }
      if (!message.empty()) {
        throw Exception(message.c_str());
      }
    } catch (const ptr::ptree_error&) {
    }
    Base::CheckErrorResponse(response, responseContent, attemptNumber);
  }

  FloodControl& GetFloodControl() const override {
    static auto result = CreateDisabledFloodControl();
    return *result;
  }
};

class PublicRequest : public Request {
 public:
  typedef Request Base;

  explicit PublicRequest(const std::string& request,
                         const std::string& params,
                         const Context& context,
                         ModuleEventsLog& log)
      : Base(request, net::HTTPRequest::HTTP_GET, params, context, log) {}

  ~PublicRequest() override = default;

 protected:
  bool IsPriority() const override { return false; }
};

class PrivateRequest : public Request {
 public:
  typedef Request Base;

  explicit PrivateRequest(const std::string& request,
                          const std::string& method,
                          const Settings& settings,
                          const bool isPriority,
                          const std::string& uriParams,
                          const Context& context,
                          ModuleEventsLog& log,
                          ModuleTradingLog* tradingLog = nullptr)
      : Base(request, method, uriParams, context, log, tradingLog),
        m_settings(settings),
        m_isPriority(isPriority) {}

  ~PrivateRequest() override = default;

 protected:
  void PrepareRequest(const net::HTTPClientSession& session,
                      const std::string& body,
                      net::HTTPRequest& request) const override {
    const auto& timestamp = boost::lexical_cast<std::string>(
        pt::to_time_t(pt::second_clock::universal_time()));
    {
      const auto& digest = Hmac::CalcSha256Digest(
          timestamp + GetRequest().getMethod() + GetRequest().getURI() + body,
          &m_settings.apiSecret[0], m_settings.apiSecret.size());
      request.set("CB-ACCESS-SIGN",
                  Base64::Encode(&digest[0], digest.size(), false));
    }
    request.set("CB-ACCESS-TIMESTAMP", timestamp);
    request.set("CB-ACCESS-KEY", m_settings.apiKey);
    request.set("CB-ACCESS-PASSPHRASE", m_settings.apiPassphrase);
    if (!body.empty()) {
      request.setContentType("application/json");
    }
    Base::PrepareRequest(session, body, request);
  }

  bool IsPriority() const override { return m_isPriority; }

 private:
  const Settings& m_settings;
  const bool m_isPriority;
};

std::string RestoreSymbol(const std::string& source) {
  return boost::replace_first_copy(source, "-", "_");
}
}  // namespace

////////////////////////////////////////////////////////////////////////////////

namespace {

class Exchange : public TradingSystem, public MarketDataSource {
  typedef boost::mutex SecuritiesMutex;
  typedef SecuritiesMutex::scoped_lock SecuritiesLock;

 public:
  explicit Exchange(const App&,
                    const TradingMode& mode,
                    Context& context,
                    std::string instanceName,
                    std::string title,
                    const ptr::ptree& conf)
      : TradingSystem(mode, context, instanceName, title),
        MarketDataSource(context, std::move(instanceName), std::move(title)),
        m_settings(conf, GetTsLog()),
        m_serverTimeDiff(
            GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
        m_isConnected(false),
        m_dataSession(
            CreateSession("api.pro.coinbase.com", m_settings, false)),
        m_tradingSession(
            CreateSession("api.pro.coinbase.com", m_settings, true)),
        m_balances(*this, GetTsLog(), GetTsTradingLog()),
        m_pollingTask(boost::make_unique<PollingTask>(
            m_settings.pollingSettings, GetMdsLog())) {}

  Exchange(Exchange&&) = default;
  Exchange(const Exchange&) = delete;
  Exchange& operator=(Exchange&&) = delete;
  Exchange& operator=(const Exchange&) = delete;

  ~Exchange() override {
    try {
      {
        boost::mutex::scoped_lock lock(m_marketDataConnectionMutex);
        auto connection = std::move(m_marketDataConnection);
        lock.unlock();
      }
      m_pollingTask.reset();
      // Each object, that implements CreateNewSecurityObject should wait for
      // log flushing before destroying objects:
      MarketDataSource::GetTradingLog().WaitForFlush();
      TradingSystem::GetTradingLog().WaitForFlush();
    } catch (...) {
      AssertFailNoException();
      terminate();
    }
  }

  using trdk::TradingSystem::GetContext;

  TradingSystem::Log& GetTsLog() const noexcept {
    return TradingSystem::GetLog();
  }
  TradingSystem::TradingLog& GetTsTradingLog() const noexcept {
    return TradingSystem::GetTradingLog();
  }

  MarketDataSource::Log& GetMdsLog() const noexcept {
    return MarketDataSource::GetLog();
  }

  bool IsConnected() const override { return m_isConnected; }

  //! Makes connection with Market Data Source.
  void Connect() override {
    Assert(!m_marketDataConnection);
    // Implementation for trdk::MarketDataSource.
    if (IsConnected()) {
      return;
    }
    GetTsLog().Debug("Creating connection...");
    CreateConnection();
  }

  void SubscribeToSecurities() override {
    boost::mutex::scoped_lock lock(m_marketDataConnectionMutex);
    if (!m_marketDataConnection) {
      return;
    }
    if (!m_isMarketDataStarted) {
      StartConnection(*m_marketDataConnection);
      m_isMarketDataStarted = true;
    } else {
      const auto connection = std::move(m_marketDataConnection);
      ScheduleMarketDataReconnect();
      lock.unlock();
    }
  }

  Balances& GetBalancesStorage() override { return m_balances; }

  boost::optional<OrderCheckError> CheckOrder(
      const trdk::Security& security,
      const Currency& currency,
      const Qty& qty,
      const boost::optional<Price>& price,
      const OrderSide& side) const override {
    {
      const auto result =
          TradingSystem::CheckOrder(security, currency, qty, price, side);
      if (result) {
        return result;
      }
    }

    const auto& productIt = m_products.find(security.GetSymbol().GetSymbol());
    if (productIt == m_products.cend()) {
      GetTsLog().Error(R"(Failed find product for "%1%" to check order.)",
                       security);
      throw Exception("Symbol is not supported by exchange");
    }
    const auto& product = productIt->second;

    if (qty < product.minQty) {
      return OrderCheckError{product.minQty};
    }
    if (product.maxQty < qty) {
      return OrderCheckError{product.maxQty};
    }

    return boost::none;
  }

  bool CheckSymbol(const std::string& symbol) const override {
    return TradingSystem::CheckSymbol(symbol) && m_products.count(symbol) > 0;
  }

  Volume CalcCommission(const Qty& qty,
                        const Price& price,
                        const OrderSide&,
                        const trdk::Security& security) const override {
    const auto& symbol = security.GetSymbol().GetSymbol();
    const auto& fee =
        (symbol == "BTC_USD" || symbol == "BCH_BTC" || symbol == "BCH_EUR" ||
         symbol == "BCH_USD" || symbol == "BTC_EUR" || symbol == "BTC_GBP")
            ? .25
            : .3;
    return (qty * price) * (fee / 100);
  }

  const boost::unordered_set<std::string>& GetSymbolListHint() const override {
    return m_symbolListHint;
  }

 protected:
  void CreateConnection() override {
    Assert(!m_marketDataConnection);

    const boost::mutex::scoped_lock lock(m_marketDataConnectionMutex);
    auto marketDataConnection = boost::make_unique<MarketDataConnection>();

    try {
      marketDataConnection->Connect();
      UpdateBalances();
      RequestProducts();
    } catch (const std::exception& ex) {
      throw ConnectError(ex.what());
    }

    m_pollingTask->AddTask(
        "Actual orders", 0,
        [this]() {
          UpdateOrders();
          return true;
        },
        m_settings.pollingSettings.GetActualOrdersRequestFrequency(), true);
    m_pollingTask->AddTask(
        "Balances", 1,
        [this]() {
          UpdateBalances();
          return true;
        },
        m_settings.pollingSettings.GetBalancesRequestFrequency(), true);

    m_marketDataConnection = std::move(marketDataConnection);

    m_isConnected = true;
  }

  trdk::Security& CreateNewSecurityObject(const Symbol& symbol) override {
    const auto& product = m_products.find(symbol.GetSymbol());
    if (product == m_products.cend()) {
      boost::format message(
          "Symbol \"%1%\" is not in the exchange product list");
      message % symbol.GetSymbol();
      throw SymbolIsNotSupportedException(message.str().c_str());
    }

    const auto result = boost::make_shared<Rest::Security>(
        GetContext(), symbol, *this,
        Rest::Security::SupportedLevel1Types()
            .set(LEVEL1_TICK_BID_PRICE)
            .set(LEVEL1_TICK_BID_QTY)
            .set(LEVEL1_TICK_ASK_PRICE)
            .set(LEVEL1_TICK_ASK_QTY));
    result->SetTradingSessionState(pt::not_a_date_time, true);

    {
      const SecuritiesLock lock(m_securitiesMutex);
      Verify(
          m_securities.emplace(product->second.id, SecuritySubscription{result})
              .second);
    }

    return *result;
  }

  std::unique_ptr<OrderTransactionContext> SendOrderTransaction(
      trdk::Security& security,
      const Currency& currency,
      const Qty& qty,
      const boost::optional<Price>& price,
      const OrderParams& params,
      const OrderSide& side,
      const TimeInForce& tif) override {
    static_assert(numberOfTimeInForces == 5, "List changed.");
    switch (tif) {
      case TIME_IN_FORCE_IOC:
        return SendOrderTransactionAndEmulateIoc(security, currency, qty, price,
                                                 params, side);
      case TIME_IN_FORCE_GTC:
        break;
      default:
        throw Exception("Order time-in-force type is not supported");
    }
    if (currency != security.GetSymbol().GetCurrency()) {
      throw Exception("Trading system supports only security quote currency");
    }
    if (!price) {
      throw Exception("Market order is not supported");
    }

    const auto& product = m_products.find(security.GetSymbol().GetSymbol());
    if (product == m_products.cend()) {
      throw Exception("Symbol is not supported by exchange");
    }

    PrivateRequest request("orders", net::HTTPRequest::HTTP_POST, m_settings,
                           true, std::string(), GetContext(), GetTsLog(),
                           &GetTsTradingLog());
    {
      boost::format requestParams(
          "{\"side\": \"%1%\", \"product_id\": \"%2%\", \"price\": \"%3%\", "
          "\"size\": \"%4%\"}");
      requestParams % (side == ORDER_SIDE_SELL ? "sell" : "buy")  // 1
          % product->second.id                                    // 2
          % RoundByPrecisionPower(price->Get(),
                                  product->second.precisionPower)  // 3
          % qty;                                                   // 4
      request.SetBody(requestParams.str());
    }

    const auto& result = request.Send(m_tradingSession);
    try {
      return boost::make_unique<OrderTransactionContext>(
          *this, boost::get<1>(result).get<OrderId>("id"));
    } catch (const ptr::ptree_error& ex) {
      boost::format error(
          R"(Wrong server response to the request "%1%" (%2%): "%3%")");
      error % request.GetName()            // 1
          % request.GetRequest().getURI()  // 2
          % ex.what();                     // 3
      throw Exception(error.str().c_str());
    }
  }

  void SendCancelOrderTransaction(
      const OrderTransactionContext& transaction) override {
    PrivateRequest(
        "orders/" + boost::lexical_cast<std::string>(transaction.GetOrderId()),
        net::HTTPRequest::HTTP_DELETE, m_settings, true, std::string(),
        GetContext(), GetTsLog(), &GetTsTradingLog())
        .Send(m_tradingSession);
  }

  void OnTransactionSent(const OrderTransactionContext& transaction) override {
    TradingSystem::OnTransactionSent(transaction);
    m_pollingTask->AccelerateNextPolling();
  }

 private:
  void RequestProducts() {
    boost::unordered_map<std::string, Product> products;
    PublicRequest request("products", "", GetContext(), GetTsLog());
    const auto response = boost::get<1>(request.Send(m_dataSession));
    for (const auto& node : response) {
      const auto& productNode = node.second;
      Product product = {productNode.get<std::string>("id"),
                         productNode.get<Qty>("base_min_size"),
                         productNode.get<Qty>("base_max_size"),
                         ConvertTickSizeToPrecisionPower(
                             productNode.get<std::string>("quote_increment"))};
      const auto& symbol = RestoreSymbol(product.id);
      const auto& productIt =
          products.emplace(std::move(symbol), std::move(product));
      if (!productIt.second) {
        GetTsLog().Error("Product duplicate: \"%1%\"", productIt.first->first);
        Assert(productIt.second);
      }
    }

    boost::unordered_set<std::string> symbolListHint;
    for (const auto& product : products) {
      symbolListHint.insert(product.first);
    }

    m_products = std::move(products);
    m_symbolListHint = std::move(symbolListHint);
  }

  void UpdateBalances() {
    PrivateRequest request("accounts", net::HTTPRequest::HTTP_GET, m_settings,
                           false, std::string(), GetContext(), GetTsLog());
    try {
      const auto response = boost::get<1>(request.Send(m_dataSession));
      for (const auto& node : response) {
        const auto& account = node.second;
        m_balances.Set(account.get<std::string>("currency"),
                       account.get<Volume>("available"),
                       account.get<Volume>("hold"));
      }
    } catch (const std::exception& ex) {
      boost::format error("Failed to request accounts: \"%1%\"");
      error % ex.what();
      throw Exception(error.str().c_str());
    }
  }

  void UpdateOrder(const ptr::ptree& order) {
    const auto& orderId = order.get<OrderId>("id");

    const auto& qty = order.get<Qty>("size");
    const auto& filledQty = order.get<Qty>("filled_size");
    AssertGe(qty, filledQty);
    const auto& remainingQty = qty - filledQty;

    const auto& time = ParseTime(order);

    const auto& status = order.get<std::string>("status");
    if (status == "open" || status == "pending") {
      remainingQty == 0 ? OnOrderFilled(time, orderId, ParseFee(order))
                        : OnOrderOpened(time, orderId);
    } else if (status == "done") {
      const auto& fee = ParseFee(order);
      remainingQty ? OnOrderCanceled(time, orderId, remainingQty, fee)
                   : OnOrderFilled(time, orderId, fee);
    } else {
      OnOrderError(time, orderId, remainingQty, ParseFee(order),
                   "Unknown order status");
    }
  }

  void UpdateOrders() {
    class OrderStatusRequest : public PrivateRequest {
     public:
      typedef PrivateRequest Base;

      explicit OrderStatusRequest(const OrderId& orderId,
                                  const Settings& settings,
                                  const Context& context,
                                  ModuleEventsLog& log,
                                  ModuleTradingLog& tradingLog)
          : Base("orders/" + orderId.GetValue(),
                 net::HTTPRequest::HTTP_GET,
                 settings,
                 false,
                 std::string(),
                 context,
                 log,
                 &tradingLog) {}

      OrderStatusRequest(OrderStatusRequest&&) = delete;
      OrderStatusRequest(const OrderStatusRequest&) = delete;
      OrderStatusRequest& operator=(OrderStatusRequest&&) = delete;
      OrderStatusRequest& operator=(const OrderStatusRequest&) = delete;

      ~OrderStatusRequest() override = default;

     protected:
      void CheckErrorResponse(const net::HTTPResponse& response,
                              const std::string& responseContent,
                              size_t attemptNumber) const override {
        try {
          const auto& message =
              ReadJson(responseContent).get<std::string>("message");
          if (message == "NotFound") {
            throw OrderIsUnknownException(message.c_str());
          }
          if (!message.empty()) {
            throw Exception(message.c_str());
          }
        } catch (const ptr::ptree_error&) {
        }
        Base::CheckErrorResponse(response, responseContent, attemptNumber);
      }
    };

    for (const auto& context : GetActiveOrderContextList()) {
      const auto& orderId = context->GetOrderId();
      OrderStatusRequest request(orderId, m_settings, GetContext(), GetTsLog(),
                                 GetTsTradingLog());
      try {
        UpdateOrder(boost::get<1>(request.Send(m_dataSession)));
      } catch (const OrderIsUnknownException&) {
        OnOrderCanceled(GetContext().GetCurrentTime(), orderId, boost::none,
                        boost::none);
      } catch (const std::exception& ex) {
        boost::format error("Failed to update order list: \"%1%\"");
        error % ex.what();
        throw Exception(error.str().c_str());
      }
    }
  }

  pt::ptime ParseTime(const ptr::ptree& order) const {
    {
      const auto& doneTimeField = order.get_optional<std::string>("done_at");
      if (doneTimeField) {
        const std::string& value = *doneTimeField;
        return pt::ptime(gr::from_string(value.substr(0, 10)),
                         pt::duration_from_string(value.substr(11, 8))) -
               m_serverTimeDiff;
      }
    }
    {
      const auto& timeField = order.get<std::string>("created_at");
      return pt::ptime(gr::from_string(timeField.substr(0, 10)),
                       pt::duration_from_string(timeField.substr(11, 8))) -
             m_serverTimeDiff;
    }
  }

  void UpdatePrices(const pt::ptime& time,
                    const ptr::ptree& message,
                    const Milestones& delayMeasurement) {
    const auto& getSecurity = [this, &message]() -> SecuritySubscription& {
      const auto& productId = message.get<std::string>("product_id");
      const auto& securityIt = m_securities.find(productId);
      if (securityIt == m_securities.cend()) {
        boost::format error(
            "Received depth-update for unknown product \"%1%\"");
        error % productId;  // 1
        throw Exception(error.str().c_str());
      }
      return securityIt->second;
    };

    const auto& type = message.get<std::string>("type");
    if (type == "snapshot") {
      UpdatePricesBySnapshot(time, getSecurity(), message, delayMeasurement);
    } else if (type == "l2update") {
      UpdatePricesIncrementally(time, getSecurity(), message, delayMeasurement);
    }
  }

  void UpdatePricesBySnapshot(const pt::ptime& time,
                              SecuritySubscription& security,
                              const ptr::ptree& message,
                              const Milestones& delayMeasurement) {
    auto bids = ReadBook<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
        message.get_child("bids"));
    security.asks = ReadBook<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
        message.get_child("asks"));
    security.bids = std::move(bids);
    FlushPrices(time, security, delayMeasurement);
  }

  void UpdatePricesIncrementally(const pt::ptime& time,
                                 SecuritySubscription& security,
                                 const ptr::ptree& message,
                                 const Milestones& delayMeasurement) {
    for (const auto& line : message.get_child("changes")) {
      boost::optional<bool> isBid;
      boost::optional<Price> price;
      for (const auto& item : line.second) {
        if (!isBid) {
          const auto& type = item.second.get_value<std::string>();
          if (type == "buy") {
            isBid = true;
          } else if (type == "sell") {
            isBid = false;
          } else {
            boost::format error(
                "Received depth-update with unknown side \"%1%\"");
            error % type;  // 1
            throw Exception(error.str().c_str());
          }
        } else if (!price) {
          price.emplace(item.second.get_value<Price>());
        } else {
          const auto& qty = item.second.get_value<Qty>();
          auto& storage = *isBid ? security.bids : security.asks;
          auto it = storage.find(*price);
          if (it == storage.cend()) {
            if (qty != 0) {
              storage.emplace(
                  *price,
                  *isBid
                      ? std::make_pair(
                            Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
                                *price),
                            Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(qty))
                      : std::make_pair(
                            Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
                                *price),
                            Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(qty)));
            }
            continue;
          }
          if (qty == 0) {
            storage.erase(it);
          } else {
            it->second.second =
                *isBid ? Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(qty)
                       : Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(qty);
          }
        }
      }
    }
    FlushPrices(time, security, delayMeasurement);
  }

  void FlushPrices(const pt::ptime& time,
                   SecuritySubscription& security,
                   const Milestones& delayMeasurement) {
    if (security.bids.empty() || security.asks.empty()) {
      security.security->SetOnline(pt::not_a_date_time, false);
      return;
    }
    const auto& bestBid = security.bids.crbegin()->second;
    const auto& bestAsk = security.asks.cbegin()->second;
    security.security->SetLevel1(time, bestBid.first, bestBid.second,
                                 bestAsk.first, bestAsk.second,
                                 delayMeasurement);
    security.security->SetOnline(pt::not_a_date_time, true);
  }

#if 0
  void UpdateBars(const std::string& productId, Rest::Security& security) {
    for (const auto& startedBar : security.GetStartedBars()) {
      const auto& startTime = startedBar.second;
      const auto endTime = GetContext().GetCurrentTime();
      if (endTime - startTime < pt::seconds(15)) {
        continue;
      }
      boost::format requestParams("start=%1%&end=%2%&granularity=%3%");
      requestParams %
          pt::to_iso_extended_string(startTime + m_serverTimeDiff)  // 1
          % pt::to_iso_extended_string(endTime + m_serverTimeDiff)  // 2
          % startedBar.first.total_seconds();                       // 3
      const auto response = boost::get<1>(
          PublicRequest("products/" + productId + "/candles",
                        requestParams.str(), GetContext(), GetTsLog())
              .Send(m_marketDataSession));
      std::vector<Bar> bars;
      for (const auto& node : response) {
        Bar bar = {};
        for (const auto& dataNode : node.second) {
          const auto& data = dataNode.second;
          if (bar.time == pt::not_a_date_time) {
            bar.time =
                pt::from_time_t(data.get_value<time_t>()) - m_serverTimeDiff;
            if (bar.time < startTime) {
              break;
            }
          } else if (!bar.lowPrice) {
            bar.lowPrice = data.get_value<Price>();
          } else if (!bar.highPrice) {
            bar.highPrice = data.get_value<Price>();
          } else if (!bar.openPrice) {
            bar.openPrice = data.get_value<Price>();
          } else if (!bar.closePrice) {
            bar.closePrice = data.get_value<Price>();
          } else if (!bar.volume) {
            bar.volume = data.get_value<Volume>();
          } else {
            AssertFail("Too many fields");
          }
        }
        if (!bar.openPrice) {
          break;
        }
        AssertLe(*bar.openPrice, *bar.highPrice);
        AssertGe(*bar.openPrice, *bar.lowPrice);
        AssertLe(*bar.closePrice, *bar.highPrice);
        AssertGe(*bar.closePrice, *bar.lowPrice);
        AssertLe(*bar.lowPrice, *bar.highPrice);
        Assert(bars.empty() || bars.back().time > bar.time);
        bars.emplace_back(std::move(bar));
      }
      security.SetBarsStartTime(startedBar.first, endTime);
      for (const auto& bar : boost::adaptors::reverse(bars)) {
        security.UpdateBar(bar);
      }
    }
  }
#endif

  void StartConnection(MarketDataConnection& connection) {
    connection.Start(
        m_securities,
        MarketDataConnection::Events{
            [this]() -> MarketDataConnection::EventInfo {
              const auto& context = GetContext();
              return {context.GetCurrentTime(),
                      context.StartStrategyTimeMeasurement()};
            },
            [this](const MarketDataConnection::EventInfo& info,
                   const ptr::ptree& message) {
              UpdatePrices(info.readTime, message, info.delayMeasurement);
            },
            [this]() {
              const boost::mutex::scoped_lock lock(m_marketDataConnectionMutex);
              if (!m_marketDataConnection) {
                GetMdsLog().Debug("Disconnected.");
                return;
              }
              const boost::shared_ptr<MarketDataConnection> connection(
                  std::move(m_marketDataConnection));
              GetMdsLog().Warn("Connection lost.");
              GetContext().GetTimer().Schedule(
                  [this, connection]() {
                    {
                      const boost::mutex::scoped_lock lock(
                          m_marketDataConnectionMutex);
                    }
                    ScheduleMarketDataReconnect();
                  },
                  m_timerScope);
            },
            [this](const std::string& event) {
              GetMdsLog().Debug(event.c_str());
            },
            [this](const std::string& event) {
              GetMdsLog().Info(event.c_str());
            },
            [this](const std::string& event) {
              GetMdsLog().Warn(event.c_str());
            },
            [this](const std::string& event) {
              GetMdsLog().Error(event.c_str());
            }});
  }

  void ScheduleMarketDataReconnect() {
    GetContext().GetTimer().Schedule(
        [this]() {
          const boost::mutex::scoped_lock lock(m_marketDataConnectionMutex);
          GetMdsLog().Info("Reconnecting...");
          Assert(!m_marketDataConnection);
          auto connection = boost::make_unique<MarketDataConnection>();
          try {
            connection->Connect();
          } catch (const std::exception& ex) {
            GetMdsLog().Error("Failed to connect: \"%1%\".", ex.what());
            ScheduleMarketDataReconnect();
            return;
          }
          StartConnection(*connection);
          m_marketDataConnection = std::move(connection);
        },
        m_timerScope);
  }

  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  bool m_isConnected;
  std::unique_ptr<net::HTTPSClientSession> m_dataSession;
  std::unique_ptr<net::HTTPSClientSession> m_tradingSession;

  SecuritiesMutex m_securitiesMutex;
  boost::unordered_map<ProductId, SecuritySubscription> m_securities;

  BalancesContainer m_balances;

  std::unique_ptr<PollingTask> m_pollingTask;

  boost::unordered_map<std::string, Product> m_products;
  boost::unordered_set<std::string> m_symbolListHint;

  boost::mutex m_marketDataConnectionMutex;
  bool m_isMarketDataStarted = false;
  std::unique_ptr<MarketDataConnection> m_marketDataConnection;

  trdk::Timer::Scope m_timerScope;

};  // namespace
}  // namespace

////////////////////////////////////////////////////////////////////////////////

TradingSystemAndMarketDataSourceFactoryResult CreateTradingSystem(
    const TradingMode& mode,
    Context& context,
    std::string instanceName,
    std::string title,
    const ptr::ptree& configuration) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  const auto& result = boost::make_shared<Exchange>(
      App::GetInstance(), mode, context, std::move(instanceName),
      std::move(title), configuration);
  return {result, result};
}

boost::shared_ptr<MarketDataSource> CreateMarketDataSource(
    Context& context,
    std::string instanceName,
    std::string title,
    const ptr::ptree& configuration) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  return boost::make_shared<Exchange>(App::GetInstance(), TRADING_MODE_LIVE,
                                      context, std::move(instanceName),
                                      std::move(title), configuration);
}

TradingSystemAndMarketDataSourceFactoryResult
CreateTradingSystemAndMarketDataSource(
    const TradingMode& mode,
    Context& context,
    std::string instanceName,
    std::string title,
    const boost::property_tree::ptree& config) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  const auto result = boost::make_shared<Exchange>(
      App::GetInstance(), mode, context, std::move(instanceName),
      std::move(title), config);
  return {result, result};
}

////////////////////////////////////////////////////////////////////////////////
