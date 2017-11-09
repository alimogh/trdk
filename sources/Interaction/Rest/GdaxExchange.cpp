/*******************************************************************************
 *   Created: 2017/10/26 14:44:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Prec.hpp"
#include "App.hpp"
#include "PollingTask.hpp"
#include "Request.hpp"
#include "Security.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Lib::Crypto;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace pc = Poco;
namespace net = pc::Net;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

namespace {

struct Settings {
  std::string apiKey;
  std::vector<unsigned char> apiSecret;
  std::string apiPassphrase;
  pt::time_duration pollingInterval;

  explicit Settings(const IniSectionRef &conf, ModuleEventsLog &log)
      : apiKey(conf.ReadKey("api_key")),
        apiSecret(Base64::Decode(conf.ReadKey("api_secret"))),
        apiPassphrase(conf.ReadKey("api_passphrase")),
        pollingInterval(pt::milliseconds(
            conf.ReadTypedKey<long>("polling_interval_milliseconds"))) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) {
    log.Info(
        "API key: \"%1%\". API secret: %2%. API passphrase: %3%. Polling "
        "interval: %4%.",
        apiKey,                                        // 1
        apiSecret.empty() ? "not set" : "is set",      // 2
        apiPassphrase.empty() ? "not set" : "is set",  // 3
        pollingInterval);                              // 4
  }

  void Validate() {}
};

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
template <Level1TickType priceType, Level1TickType qtyType>
std::pair<Level1TickValue, Level1TickValue> ReadTopOfBook(
    const ptr::ptree &source) {
  for (const auto &lines : source) {
    double price;
    size_t i = 0;
    for (const auto &val : lines.second) {
      if (i == 0) {
        price = val.second.get_value<double>();
        ++i;
      } else {
        AssertEq(1, i);
        return {
            Level1TickValue::Create<priceType>(std::move(price)),
            Level1TickValue::Create<qtyType>(val.second.get_value<double>())};
      }
    }
    break;
  }
  return {Level1TickValue::Create<priceType>(
              std::numeric_limits<double>::quiet_NaN()),
          Level1TickValue::Create<qtyType>(
              std::numeric_limits<double>::quiet_NaN())};
}
#pragma warning(pop)
}

////////////////////////////////////////////////////////////////////////////////

namespace {

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

 public:
  explicit Request(const std::string &request,
                   const std::string &method,
                   const std::string &uriParams)
      : Base("/" + request, request, method, uriParams) {}

  virtual ~Request() override = default;

 protected:
  virtual void Request::CheckResponce(
      const net::HTTPResponse &response,
      std::istream &responseStream) const override {
    if (response.getStatus() == net::HTTPResponse::HTTP_OK) {
      Base::CheckResponce(response, responseStream);
      return;
    }
    try {
      ptr::ptree result;
      ptr::read_json(responseStream, result);
      const auto &message = result.get<std::string>("message");
      if (message == "Order already done") {
        throw TradingSystem::OrderIsUnknown(message.c_str());
      }
    } catch (const ptr::ptree_error &) {
      Base::CheckResponce(response, responseStream);
    }
  }
};

class PublicRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PublicRequest(const std::string &request)
      : Base(request, net::HTTPRequest::HTTP_GET, std::string()) {}

  virtual ~PublicRequest() override = default;
};

class PrivateRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PrivateRequest(const std::string &request,
                          const std::string &method,
                          const Settings &settings,
                          const std::string &uriParams = std::string())
      : Base(request, method, uriParams), m_settings(settings) {}

  virtual ~PrivateRequest() override = default;

 protected:
  virtual void PrepareRequest(const net::HTTPClientSession &session,
                              const std::string &body,
                              net::HTTPRequest &request) const override {
    const auto &timestamp = boost::lexical_cast<std::string>(
        pt::to_time_t(pt::second_clock::universal_time()));
    {
      const auto &digest = Hmac::CalcSha256Digest(
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

 private:
  const Settings &m_settings;
};

std::string NormilizeSymbol(const std::string &source) {
  return boost::replace_first_copy(source, "_", "-");
}
}

////////////////////////////////////////////////////////////////////////////////

namespace {

class GdaxExchange : public TradingSystem, public MarketDataSource {
 private:
  typedef boost::mutex SecuritiesMutex;
  typedef SecuritiesMutex::scoped_lock SecuritiesLock;

  struct SecuritySubscribtion {
    boost::shared_ptr<Rest::Security> security;
    boost::shared_ptr<Request> request;
    bool isSubscribed;
  };

  struct Order {
    OrderId id;
    boost::optional<trdk::Price> price;
    Qty qty;
    Qty remainingQty;
    std::string symbol;
    OrderSide side;
    TimeInForce tif;
    pt::ptime creationTime;
    pt::ptime doneTime;
    OrderStatus status;
  };

 public:
  explicit GdaxExchange(const App &,
                        const TradingMode &mode,
                        size_t tradingSystemIndex,
                        size_t marketDataSourceIndex,
                        Context &context,
                        const std::string &instanceName,
                        const IniSectionRef &conf)
      : TradingSystem(mode, tradingSystemIndex, context, instanceName),
        MarketDataSource(marketDataSourceIndex, context, instanceName),
        m_settings(conf, GetTsLog()),
        m_isConnected(false),
        m_marketDataSession("api.gdax.com"),
        m_tradingSession(m_marketDataSession.getHost()),
        m_orderTransactionRequest(
            "orders", net::HTTPRequest::HTTP_POST, m_settings),
        m_orderListRequest("orders",
                           net::HTTPRequest::HTTP_GET,
                           m_settings,
                           "status=open&status=pending"),
        m_numberOfPulls(0) {
    m_marketDataSession.setKeepAlive(true);
    m_tradingSession.setKeepAlive(true);
  }

  virtual ~GdaxExchange() override = default;

 public:
  using trdk::TradingSystem::GetContext;

  TradingSystem::Log &GetTsLog() const noexcept {
    return TradingSystem::GetLog();
  }

  MarketDataSource::Log &GetMdsLog() const noexcept {
    return MarketDataSource::GetLog();
  }

 public:
  virtual bool IsConnected() const override { return m_isConnected; }

  //! Makes connection with Market Data Source.
  virtual void Connect(const IniSectionRef &conf) override {
    // Implementation for trdk::MarketDataSource.
    if (IsConnected()) {
      return;
    }
    GetTsLog().Info("Creating connection...");
    CreateConnection(conf);
  }

  virtual void SubscribeToSecurities() override {
    {
      const SecuritiesLock lock(m_securitiesMutex);
      for (auto &subscribtion : m_securities) {
        if (subscribtion.second.isSubscribed) {
          continue;
        }
        GetMdsLog().Info("Starting Market Data subscribtion for \"%1%\"...",
                         *subscribtion.second.security);
        subscribtion.second.isSubscribed = true;
      }
    }

    if (m_pollingTask) {
      return;
    }
    m_pollingTask = boost::make_unique<PollingTask>(
        [this]() {
          const SecuritiesLock lock(m_securitiesMutex);
          for (const auto &subscribtion : m_securities) {
            if (!subscribtion.second.isSubscribed) {
              continue;
            }
            auto &security = *subscribtion.second.security;
            auto &request = *subscribtion.second.request;
            const auto &response =
                request.Send(m_marketDataSession, GetContext());
            const auto &time = boost::get<0>(response);
            const auto &update = boost::get<1>(response);
            const auto &delayMeasurement = boost::get<2>(response);
            try {
              const auto &bestBid =
                  ReadTopOfBook<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
                      update.get_child("bids"));
              const auto &bestAsk =
                  ReadTopOfBook<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
                      update.get_child("asks"));
              security.SetLevel1(time, std::move(bestBid.first),
                                 std::move(bestBid.second),
                                 std::move(bestAsk.first),
                                 std::move(bestAsk.second), delayMeasurement);
            } catch (const std::exception &ex) {
              boost::format error(
                  "Failed to read order book state for \"%1%\": \"%2%\"");
              error % security  // 1
                  % ex.what();  // 2
              throw MarketDataSource::Error(error.str().c_str());
            }
          }
          UpdateOrders();
          if (!(++m_numberOfPulls % 20)) {
            RequestOpenedOrders();
          }
        },
        m_settings.pollingInterval, GetMdsLog());
  }

 protected:
  virtual void CreateConnection(const IniSectionRef &) override {
    try {
      // RequestAccounts();
    } catch (const std::exception &ex) {
      GetTsLog().Error(ex.what());
    }
    try {
      RequestOpenedOrders();
    } catch (const std::exception &ex) {
      GetTsLog().Error(ex.what());
    }
    m_isConnected = true;
  }

  virtual trdk::Security &CreateNewSecurityObject(
      const Symbol &symbol) override {
    {
      const SecuritiesLock lock(m_securitiesMutex);
      const auto &it = m_securities.find(symbol);
      if (it != m_securities.cend()) {
        return *it->second.security;
      }
    }

    const auto result = boost::make_shared<Rest::Security>(
        GetContext(), symbol, *this,
        Rest::Security::SupportedLevel1Types()
            .set(LEVEL1_TICK_BID_PRICE)
            .set(LEVEL1_TICK_BID_QTY)
            .set(LEVEL1_TICK_ASK_PRICE)
            .set(LEVEL1_TICK_ASK_QTY));
    result->SetOnline(pt::not_a_date_time, true);
    result->SetTradingSessionState(pt::not_a_date_time, true);

    {
      const auto request = boost::make_shared<PublicRequest>(
          "products/" + NormilizeSymbol(result->GetSymbol().GetSymbol()) +
          "/book/");
      const SecuritiesLock lock(m_securitiesMutex);
      Verify(m_securities.emplace(symbol, SecuritySubscribtion{result, request})
                 .second);
    }

    return *result;
  }

  virtual std::unique_ptr<OrderTransactionContext> SendOrderTransaction(
      trdk::Security &security,
      const Currency &currency,
      const Qty &qty,
      const boost::optional<Price> &price,
      const OrderParams &params,
      const OrderSide &side,
      const TimeInForce &tif) override {
    static_assert(numberOfTimeInForces == 5, "List changed.");
    switch (tif) {
      case TIME_IN_FORCE_IOC:
        return SendOrderTransactionAndEmulateIoc(security, currency, qty, price,
                                                 params, side);
      case TIME_IN_FORCE_GTC:
        break;
      default:
        throw TradingSystem::Error("Order time-in-force type is not supported");
    }
    if (currency != security.GetSymbol().GetCurrency()) {
      throw TradingSystem::Error(
          "Trading system supports only security quote currency");
    }
    if (!price) {
      throw TradingSystem::Error("Market order is not supported");
    }

    {
      boost::format requestParams(
          "{\"side\": \"%1%\", \"product_id\": \"%2%\", \"price\": \"%3$.8f\", "
          "\"size\": \"%4$.8f\"}");
      requestParams % (side == ORDER_SIDE_SELL ? "sell" : "buy")  // 1
          % NormilizeSymbol(security.GetSymbol().GetSymbol())     // 2
          % *price                                                // 3
          % qty;                                                  // 4
      m_orderTransactionRequest.SetBody(requestParams.str());
    }

    const auto &result =
        m_orderTransactionRequest.Send(m_tradingSession, GetContext());
    MakeServerAnswerDebugDump(boost::get<1>(result), *this);

    try {
      return boost::make_unique<OrderTransactionContext>(
          boost::get<1>(result).get<OrderId>("id"));
    } catch (const std::exception &ex) {
      boost::format error("Failed to read order transaction reply: \"%1%\"");
      error % ex.what();
      throw TradingSystem::Error(error.str().c_str());
    }
  }

  virtual void SendCancelOrderTransaction(const OrderId &orderId) override {
    PrivateRequest request("orders/" + orderId.GetValue(),
                           net::HTTPRequest::HTTP_DELETE, m_settings);
    const auto &result = request.Send(m_tradingSession, GetContext());
    MakeServerAnswerDebugDump(boost::get<1>(result), *this);
  }

 private:
  void RequestAccounts() {
    PrivateRequest request("accounts", net::HTTPRequest::HTTP_GET, m_settings);
    try {
      const auto response =
          boost::get<1>(request.Send(m_tradingSession, GetContext()));
      MakeServerAnswerDebugDump(response, *this);
      for (const auto &node : response) {
        const auto &account = node.second;
        GetTsLog().Info(
            "%1% balance: %2$.8f (available: %3$.8f, hold: %4$.8f).",
            account.get<std::string>("currency"),  // 1
            account.get<double>("balance"),        // 2
            account.get<double>("available"),      // 3
            account.get<double>("hold"));          // 4
      }
    } catch (const std::exception &ex) {
      GetTsLog().Error("Failed to request accounts: \"%1%\".", ex.what());  // 1
      throw Exception("Failed to request accounts");
    }
  }

  void UpdateOrder(const Order &order, const OrderStatus &status) {
    OnOrder(order.id, order.symbol, status, order.qty, order.remainingQty,
            order.price, order.side, order.tif, order.creationTime,
            order.doneTime);
  }

  Order UpdateOrder(const ptr::ptree &order) {
    const auto &orderId = order.get<OrderId>("id");

    OrderSide side;
    {
      const auto &sideField = order.get<std::string>("side");
      if (sideField == "sell") {
        side = ORDER_SIDE_SELL;
      } else if (sideField == "buy") {
        side = ORDER_SIDE_BUY;
      }
    }

    boost::optional<Price> price;
    {
      const auto &type = order.get<std::string>("type");
      if (type == "limit" || type == "stop") {
        price = order.get<double>("price");
      } else if (type != "market") {
        GetTsLog().Error("Unknown order type \"%1%\" for order %2%.", type,
                         orderId);
        throw TradingSystem::Error("Failed to request order status");
      }
    }

    TimeInForce tif;
    {
      const auto &tifField = order.get<std::string>("time_in_force");
      if (tifField == "GTC" || tifField == "GTT") {
        tif = TIME_IN_FORCE_GTC;
      } else if (tifField == "IOC") {
        tif = TIME_IN_FORCE_IOC;
      } else if (tifField == "FOR") {
        tif = TIME_IN_FORCE_FOK;
      } else {
        GetTsLog().Error("Unknown order type \"%1%\" for order %2%.", tifField,
                         orderId);
        throw TradingSystem::Error("Failed to request order status");
      }
    }

    const Qty qty = order.get<double>("size");
    const Qty filledQty = order.get<double>("filled_size");

    OrderStatus status;
    {
      const auto &statusField = order.get<std::string>("status");
      if (statusField == "open") {
        status = filledQty > 0 ? ORDER_STATUS_FILLED_PARTIALLY
                               : ORDER_STATUS_SUBMITTED;
      } else if (statusField == "pending") {
        status = ORDER_STATUS_SUBMITTED;
      } else if (statusField == "done") {
        status = ORDER_STATUS_FILLED;
      } else {
        GetTsLog().Error("Unknown order status \"%1%\" for order %2%.",
                         statusField, orderId);
        throw TradingSystem::Error("Failed to request order status");
      }
    }

    pt::ptime time;
    {
      const auto &timeField = order.get<std::string>("created_at");
      time = {gr::from_string(timeField.substr(0, 10)),
              pt::duration_from_string(timeField.substr(11, 8))};
    }
    pt::ptime doneTime;
    {
      const auto &doneTimeField = order.get_optional<std::string>("done_at");
      if (doneTimeField) {
        const std::string &value = *doneTimeField;
        doneTime = {gr::from_string(value.substr(0, 10)),
                    pt::duration_from_string(value.substr(11, 8))};
      } else {
        doneTime = time;
      }
    }

    const Order result = {std::move(orderId),
                          std::move(price),
                          std::move(qty),
                          qty - filledQty,
                          order.get<std::string>("product_id"),
                          std::move(side),
                          std::move(tif),
                          time,
                          doneTime,
                          status};
    UpdateOrder(result, result.status);
    return result;
  }

  void RequestOpenedOrders() {
    boost::unordered_map<OrderId, Order> notifiedOrderOrders;
    try {
      const auto orders = boost::get<1>(
          m_orderListRequest.Send(m_marketDataSession, GetContext()));
      MakeServerAnswerDebugDump(orders, *this);
      for (const auto &order : orders) {
        const Order notifiedOrder = UpdateOrder(order.second);
        m_orders.erase(notifiedOrder.id);
        if (notifiedOrder.status != ORDER_STATUS_CANCELLED) {
          const auto id = notifiedOrder.id;
          notifiedOrderOrders.emplace(id, std::move(notifiedOrder));
        }
      }
    } catch (const std::exception &ex) {
      GetTsLog().Error("Failed to request order list: \"%1%\".", ex.what());
      throw Exception("Failed to request order list");
    }
    for (const auto &canceledOrder : m_orders) {
      UpdateOrder(canceledOrder.second, ORDER_STATUS_CANCELLED);
    }
    m_orders.swap(notifiedOrderOrders);
  }

  void UpdateOrders() {
    for (const OrderId &orderId : GetActiveOrderList()) {
      PrivateRequest request("orders/" + orderId.GetValue(),
                             net::HTTPRequest::HTTP_GET, m_settings);
      try {
        const auto result =
            boost::get<1>(request.Send(m_marketDataSession, GetContext()));
        MakeServerAnswerDebugDump(result, *this);
        UpdateOrder(result);
      } catch (const std::exception &ex) {
        GetTsLog().Error("Failed to update order list: \"%1%\".", ex.what());
        throw Exception("Failed to update order list");
      }
    }
  }

 private:
  Settings m_settings;

  bool m_isConnected;
  net::HTTPSClientSession m_marketDataSession;
  net::HTTPSClientSession m_tradingSession;

  SecuritiesMutex m_securitiesMutex;
  boost::unordered_map<Lib::Symbol, SecuritySubscribtion> m_securities;

  std::unique_ptr<PollingTask> m_pollingTask;

  PrivateRequest m_orderTransactionRequest;

  PrivateRequest m_orderListRequest;
  boost::unordered_map<OrderId, Order> m_orders;
  size_t m_numberOfPulls;

  trdk::Timer::Scope m_timerScope;
};
}

////////////////////////////////////////////////////////////////////////////////

TradingSystemAndMarketDataSourceFactoryResult CreateGdax(
    const TradingMode &mode,
    size_t tradingSystemIndex,
    size_t marketDataSourceIndex,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<GdaxExchange>(
      App::GetInstance(), mode, tradingSystemIndex, marketDataSourceIndex,
      context, instanceName, configuration);
  return {result, result};
}

boost::shared_ptr<MarketDataSource> CreateGdaxMarketDataSource(
    size_t index,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<GdaxExchange>(App::GetInstance(), TRADING_MODE_LIVE,
                                          index, index, context, instanceName,
                                          configuration);
}

////////////////////////////////////////////////////////////////////////////////
