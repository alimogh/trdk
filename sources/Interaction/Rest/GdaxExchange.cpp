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
#include "FloodControl.hpp"
#include "PollingTask.hpp"
#include "Request.hpp"
#include "Security.hpp"
#include "Settings.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Lib::Crypto;
using namespace trdk::TradingLib;
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace pc = Poco;
namespace net = pc::Net;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

namespace {

struct Settings : public Rest::Settings {
  std::string apiKey;
  std::vector<unsigned char> apiSecret;
  std::string apiPassphrase;

  explicit Settings(const IniSectionRef &conf, ModuleEventsLog &log)
      : Rest::Settings(conf, log),
        apiKey(conf.ReadKey("api_key")),
        apiSecret(Base64::Decode(conf.ReadKey("api_secret"))),
        apiPassphrase(conf.ReadKey("api_passphrase")) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) {
    log.Info("API key: \"%1%\". API secret: %2%. API passphrase: %3%.",
             apiKey,                                         // 1
             apiSecret.empty() ? "not set" : "is set",       // 2
             apiPassphrase.empty() ? "not set" : "is set");  // 3
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

struct Product {
  std::string id;
  Qty minQty;
  Qty maxQty;
  uintmax_t precisionPower;
};

Volume ParseFee(const ptr::ptree &order) {
  return order.get<Volume>("fill_fees");
}
}  // namespace

////////////////////////////////////////////////////////////////////////////////

namespace {

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

 public:
  explicit Request(const std::string &request,
                   const std::string &method,
                   const std::string &uriParams,
                   const Context &context,
                   ModuleEventsLog &log,
                   ModuleTradingLog *tradingLog = nullptr)
      : Base("/" + request,
             request,
             method,
             uriParams,
             context,
             log,
             tradingLog) {}

  virtual ~Request() override = default;

 protected:
  virtual void CheckErrorResponse(const net::HTTPResponse &response,
                                  const std::string &responseContent,
                                  size_t attemptNumber) const override {
    try {
      const auto &message =
          ReadJson(responseContent).get<std::string>("message");
      if (boost::iequals(message, "Order already done") ||
          boost::iequals(message, "order not found")) {
        throw OrderIsUnknownException(message.c_str());
      } else if (boost::iequals(message, "Insufficient funds")) {
        throw InsufficientFundsException(message.c_str());
      } else if (!message.empty()) {
        throw Exception(message.c_str());
      }
    } catch (const ptr::ptree_error &) {
    }
    Base::CheckErrorResponse(response, responseContent, attemptNumber);
  }

  virtual FloodControl &GetFloodControl() const override {
    static auto result = CreateDisabledFloodControl();
    return *result;
  }
};

class PublicRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PublicRequest(const std::string &request,
                         const Context &context,
                         ModuleEventsLog &log)
      : Base(request, net::HTTPRequest::HTTP_GET, std::string(), context, log) {
  }

  virtual ~PublicRequest() override = default;

 protected:
  virtual bool IsPriority() const { return false; }
};

class PrivateRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PrivateRequest(const std::string &request,
                          const std::string &method,
                          const Settings &settings,
                          bool isPriority,
                          const std::string &uriParams,
                          const Context &context,
                          ModuleEventsLog &log,
                          ModuleTradingLog *tradingLog = nullptr)
      : Base(request, method, uriParams, context, log, tradingLog),
        m_settings(settings),
        m_isPriority(isPriority) {}

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

  virtual bool IsPriority() const { return m_isPriority; }

 private:
  const Settings &m_settings;
  const bool m_isPriority;
};

std::string NormilizeProductId(std::string source) {
  return boost::replace_first_copy(source, "_", "-");
}
std::string RestoreSymbol(const std::string &source) {
  return boost::replace_first_copy(source, "-", "_");
}
}  // namespace

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

 public:
  explicit GdaxExchange(const App &,
                        const TradingMode &mode,
                        Context &context,
                        const std::string &instanceName,
                        const IniSectionRef &conf)
      : TradingSystem(mode, context, instanceName),
        MarketDataSource(context, instanceName),
        m_settings(conf, GetTsLog()),
        m_serverTimeDiff(
            GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
        m_isConnected(false),
        m_marketDataSession(CreateSession("api.gdax.com", m_settings, false)),
        m_tradingSession(CreateSession("api.gdax.com", m_settings, true)),
        m_balances(*this, GetTsLog(), GetTsTradingLog()),
        m_pollingTask(boost::make_unique<PollingTask>(
            m_settings.pollingSetttings, GetMdsLog())) {}

  virtual ~GdaxExchange() override {
    try {
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

 public:
  using trdk::TradingSystem::GetContext;

  TradingSystem::Log &GetTsLog() const noexcept {
    return TradingSystem::GetLog();
  }
  TradingSystem::TradingLog &GetTsTradingLog() const noexcept {
    return TradingSystem::GetTradingLog();
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
    GetTsLog().Debug("Creating connection...");
    CreateConnection(conf);
  }

  virtual void SubscribeToSecurities() override {
    {
      const SecuritiesLock lock(m_securitiesMutex);
      for (auto &subscribtion : m_securities) {
        if (subscribtion.second.isSubscribed) {
          continue;
        }
        GetMdsLog().Debug("Starting Market Data subscribtion for \"%1%\"...",
                          *subscribtion.second.security);
        subscribtion.second.isSubscribed = true;
      }
    }

    m_pollingTask->AddTask(
        "Prices", 1,
        [this]() -> bool {
          const SecuritiesLock lock(m_securitiesMutex);
          for (const auto &subscribtion : m_securities) {
            if (!subscribtion.second.isSubscribed) {
              continue;
            }
            auto &security = *subscribtion.second.security;
            auto &request = *subscribtion.second.request;
            try {
              const auto &response = request.Send(m_marketDataSession);
              const auto &time = boost::get<0>(response);
              const auto &update = boost::get<1>(response);
              const auto &delayMeasurement = boost::get<2>(response);
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
              try {
                security.SetOnline(pt::not_a_date_time, false);
              } catch (...) {
                AssertFailNoException();
                throw;
              }
              boost::format error(
                  "Failed to read order book for \"%1%\": \"%2%\"");
              error % security  // 1
                  % ex.what();  // 2
              throw Exception(error.str().c_str());
            }
            security.SetOnline(pt::not_a_date_time, true);
          }
          return true;
        },
        m_settings.pollingSetttings.GetPricesRequestFrequency(), false);

    m_pollingTask->AccelerateNextPolling();
  }

  virtual Balances &GetBalancesStorage() override { return m_balances; }

  boost::optional<OrderCheckError> CheckOrder(
      const trdk::Security &security,
      const Currency &currency,
      const Qty &qty,
      const boost::optional<Price> &price,
      const OrderSide &side) const {
    {
      const auto result =
          TradingSystem::CheckOrder(security, currency, qty, price, side);
      if (result) {
        return result;
      }
    }

    const auto &productIt = m_products.find(security.GetSymbol().GetSymbol());
    if (productIt == m_products.cend()) {
      GetTsLog().Warn("Failed find product for \"%1%\" to check order.",
                      security);
      return boost::none;
    }
    const auto &product = productIt->second;

    if (qty < product.minQty) {
      return OrderCheckError{product.minQty};
    } else if (product.maxQty < qty) {
      return OrderCheckError{product.maxQty};
    }

    return boost::none;
  }

  virtual bool CheckSymbol(const std::string &symbol) const override {
    return TradingSystem::CheckSymbol(symbol) && m_products.count(symbol) > 0;
  }

  virtual Volume CalcCommission(const Qty &qty,
                                const Price &price,
                                const OrderSide &,
                                const trdk::Security &security) const override {
    const auto &symbol = security.GetSymbol().GetSymbol();
    const auto &fee =
        (symbol == "BTC_USD" || symbol == "BCH_BTC" || symbol == "BCH_EUR" ||
         symbol == "BCH_USD" || symbol == "BTC_EUR" || symbol == "BTC_GBP")
            ? .25
            : .3;
    return (qty * price) * (fee / 100);
  }

 protected:
  virtual void CreateConnection(const IniSectionRef &) override {
    try {
      UpdateBalances();
      RequestProducts();
    } catch (const std::exception &ex) {
      throw ConnectError(ex.what());
    }

    m_pollingTask->AddTask(
        "Actual orders", 0,
        [this]() {
          UpdateOrders();
          return true;
        },
        m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
    m_pollingTask->AddTask(
        "Balances", 1,
        [this]() {
          UpdateBalances();
          return true;
        },
        m_settings.pollingSetttings.GetBalancesRequestFrequency(), true);

    m_isConnected = true;
  }

  virtual trdk::Security &CreateNewSecurityObject(
      const Symbol &symbol) override {
    const auto &product = m_products.find(symbol.GetSymbol());
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
      const auto request = boost::make_shared<PublicRequest>(
          "products/" + product->second.id + "/book/", GetContext(),
          GetTsLog());
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
        throw Exception("Order time-in-force type is not supported");
    }
    if (currency != security.GetSymbol().GetCurrency()) {
      throw Exception("Trading system supports only security quote currency");
    }
    if (!price) {
      throw Exception("Market order is not supported");
    }

    const auto &product = m_products.find(security.GetSymbol().GetSymbol());
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

    const auto &result = request.Send(m_tradingSession);
    try {
      return boost::make_unique<OrderTransactionContext>(
          *this, boost::get<1>(result).get<OrderId>("id"));
    } catch (const ptr::ptree_error &ex) {
      boost::format error(
          "Wrong server response to the request \"%1%\" (%2%): \"%3%\"");
      error % request.GetName()            // 1
          % request.GetRequest().getURI()  // 2
          % ex.what();                     // 3
      throw Exception(error.str().c_str());
    }
  }

  virtual void SendCancelOrderTransaction(
      const OrderTransactionContext &transaction) override {
    PrivateRequest(
        "orders/" + boost::lexical_cast<std::string>(transaction.GetOrderId()),
        net::HTTPRequest::HTTP_DELETE, m_settings, true, std::string(),
        GetContext(), GetTsLog(), &GetTsTradingLog())
        .Send(m_tradingSession);
  }

  virtual void OnTransactionSent(
      const OrderTransactionContext &transaction) override {
    TradingSystem::OnTransactionSent(transaction);
    m_pollingTask->AccelerateNextPolling();
  }

 private:
  void RequestProducts() {
    boost::unordered_map<std::string, Product> products;
    PublicRequest request("products", GetContext(), GetTsLog());
    const auto response = boost::get<1>(request.Send(m_marketDataSession));
    std::vector<std::string> log;
    for (const auto &node : response) {
      const auto &productNode = node.second;
      Product product = {productNode.get<std::string>("id"),
                         productNode.get<Qty>("base_min_size"),
                         productNode.get<Qty>("base_max_size")};
      const auto &quoteIncrement =
          productNode.get<std::string>("quote_increment");
      const auto dotPos = quoteIncrement.find('.');
      if (dotPos != std::string::npos && quoteIncrement.size() > dotPos) {
        product.precisionPower = quoteIncrement.size() - dotPos - 1;
        product.precisionPower = static_cast<decltype(product.precisionPower)>(
            std::pow(10, product.precisionPower));
      }
      const auto &symbol = RestoreSymbol(product.id);
      const auto &productIt =
          products.emplace(std::move(symbol), std::move(product));
      if (!productIt.second) {
        GetTsLog().Error("Product duplicate: \"%1%\"", productIt.first->first);
        Assert(productIt.second);
        continue;
      }
      boost::format logStr("%1% (ID: \"%2%\", price step: %3%)");
      logStr % productIt.first->first   // 1
          % productIt.first->second.id  // 2
          % quoteIncrement;             // 3
      log.emplace_back(logStr.str());
    }
    GetTsLog().Info("Products: %1%.", boost::join(log, ", "));
    m_products = std::move(products);
  }

  void UpdateBalances() {
    PrivateRequest request("accounts", net::HTTPRequest::HTTP_GET, m_settings,
                           false, std::string(), GetContext(), GetTsLog());
    try {
      const auto response = boost::get<1>(request.Send(m_marketDataSession));
      for (const auto &node : response) {
        const auto &account = node.second;
        m_balances.Set(account.get<std::string>("currency"),
                       account.get<Volume>("available"),
                       account.get<Volume>("hold"));
      }
    } catch (const std::exception &ex) {
      boost::format error("Failed to request accounts: \"%1%\"");
      error % ex.what();
      throw Exception(error.str().c_str());
    }
  }

  void UpdateOrder(const ptr::ptree &order) {
    const auto &orderId = order.get<OrderId>("id");

    const auto &qty = order.get<Qty>("size");
    const auto &filledQty = order.get<Qty>("filled_size");
    AssertGe(qty, filledQty);
    const auto &remainingQty = qty - filledQty;

    const auto &time = ParseTime(order);

    const auto &status = order.get<std::string>("status");
    if (status == "open" || status == "pending") {
      remainingQty == 0 ? OnOrderFilled(time, orderId, ParseFee(order))
                        : OnOrderOpened(time, orderId);
    } else if (status == "done") {
      const auto &fee = ParseFee(order);
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

     public:
      explicit OrderStatusRequest(const OrderId &orderId,
                                  const Settings &settings,
                                  const Context &context,
                                  ModuleEventsLog &log,
                                  ModuleTradingLog &tradingLog)
          : Base("orders/" + orderId.GetValue(),
                 net::HTTPRequest::HTTP_GET,
                 settings,
                 false,
                 std::string(),
                 context,
                 log,
                 &tradingLog) {}
      virtual ~OrderStatusRequest() override = default;

     protected:
      virtual void CheckErrorResponse(const net::HTTPResponse &response,
                                      const std::string &responseContent,
                                      size_t attemptNumber) const {
        try {
          const auto &message =
              ReadJson(responseContent).get<std::string>("message");
          if (message == "NotFound") {
            throw OrderIsUnknownException(message.c_str());
          } else if (!message.empty()) {
            throw Exception(message.c_str());
          }
        } catch (const ptr::ptree_error &) {
        }
        Base::CheckErrorResponse(response, responseContent, attemptNumber);
      }
    };

    for (const auto &context : GetActiveOrderContextList()) {
      const auto &orderId = context->GetOrderId();
      OrderStatusRequest request(orderId, m_settings, GetContext(), GetTsLog(),
                                 GetTsTradingLog());
      try {
        UpdateOrder(boost::get<1>(request.Send(m_marketDataSession)));
      } catch (const OrderIsUnknownException &) {
        OnOrderCanceled(GetContext().GetCurrentTime(), orderId, boost::none,
                        boost::none);
      } catch (const std::exception &ex) {
        boost::format error("Failed to update order list: \"%1%\"");
        error % ex.what();
        throw Exception(error.str().c_str());
      }
    }
  }

  pt::ptime ParseTime(const ptr::ptree &order) const {
    {
      const auto &doneTimeField = order.get_optional<std::string>("done_at");
      if (doneTimeField) {
        const std::string &value = *doneTimeField;
        return pt::ptime(gr::from_string(value.substr(0, 10)),
                         pt::duration_from_string(value.substr(11, 8))) -
               m_serverTimeDiff;
        ;
      }
    }
    {
      const auto &timeField = order.get<std::string>("created_at");
      return pt::ptime(gr::from_string(timeField.substr(0, 10)),
                       pt::duration_from_string(timeField.substr(11, 8))) -
             m_serverTimeDiff;
    }
  }

 private:
  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  bool m_isConnected;
  std::unique_ptr<net::HTTPSClientSession> m_marketDataSession;
  std::unique_ptr<net::HTTPSClientSession> m_tradingSession;

  SecuritiesMutex m_securitiesMutex;
  boost::unordered_map<Lib::Symbol, SecuritySubscribtion> m_securities;

  BalancesContainer m_balances;

  std::unique_ptr<PollingTask> m_pollingTask;

  trdk::Timer::Scope m_timerScope;

  boost::unordered_map<std::string, Product> m_products;
};
}  // namespace

////////////////////////////////////////////////////////////////////////////////

TradingSystemAndMarketDataSourceFactoryResult CreateGdax(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<GdaxExchange>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return {result, result};
}

boost::shared_ptr<MarketDataSource> CreateGdaxMarketDataSource(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<GdaxExchange>(App::GetInstance(), TRADING_MODE_LIVE,
                                          context, instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
