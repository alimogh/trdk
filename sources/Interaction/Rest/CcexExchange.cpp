/*******************************************************************************
 *   Created: 2017/10/16 22:11:08
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
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace pc = Poco;
namespace net = pc::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

namespace {

struct Endpoint : private boost::noncopyable {
  const std::string host;
  MinTimeBetweenRequestsFloodControl floodControl;

  explicit Endpoint(
      const std::string &host,
      const boost::posix_time::time_duration &minTimeBetweenRequests)
      : host(host), floodControl(minTimeBetweenRequests) {}
};
Endpoint &GetEndpoint() {
  static Endpoint result("c-cex.com", pt::seconds(1));
  return result;
}

struct Settings : public Rest::Settings {
  std::string apiKey;
  std::string apiSecret;

  explicit Settings(const IniSectionRef &conf, ModuleEventsLog &log)
      : Rest::Settings(conf, log),
        apiKey(conf.ReadKey("api_key")),
        apiSecret(conf.ReadKey("api_secret")) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) {
    log.Info("API key: \"%1%\". API secret: %2%.",
             apiKey,                                     // 1
             apiSecret.empty() ? "not set" : "is set");  // 2
  }

  void Validate() {}
};

struct CcexOrder {
  OrderId id;
  std::string symbol;
  OrderStatus status;
  Qty qty;
  Qty remainingQty;
  boost::optional<trdk::Price> price;
  OrderSide side;
  TimeInForce tif;
  pt::ptime openTime;
  pt::ptime updateTime;
};

struct Product {
  std::string id;
};
}

////////////////////////////////////////////////////////////////////////////////

namespace {

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

 public:
  explicit Request(const std::string &uri,
                   const std::string &name,
                   FloodControl &floodControl,
                   const std::string &uriParams,
                   const Context &context,
                   ModuleEventsLog &log,
                   ModuleTradingLog *tradingLog = nullptr)
      : Base(uri,
             name,
             net::HTTPRequest::HTTP_GET,
             AppendUriParams("a=" + name, uriParams),
             context,
             log,
             tradingLog),
        m_floodControl(floodControl) {}

  virtual ~Request() override = default;

 public:
  virtual boost::tuple<pt::ptime, ptr::ptree, Milestones> Send(
      net::HTTPClientSession &session) override {
    auto result = Base::Send(session);
    const auto &responseTree = boost::get<1>(result);

    {
      const auto &status = responseTree.get_optional<std::string>("success");
      if (!status || *status != "true") {
        const auto &message = responseTree.get_optional<std::string>("message");
        std::ostringstream error;
        error << "The server returned an error in response to the request \""
              << GetName() << "\" (" << GetRequest().getURI() << "): ";
        if (message) {
          if (boost::iequals(*message, "UUID_INVALID")) {
            throw OrderIsUnknown(message->c_str());
          }
          error << "\"" << *message << "\"";
        } else {
          error << "Unknown error";
        }
        error << " (status: ";
        if (status) {
          error << "\"" << *status << "\"";
        } else {
          error << "unknown";
        }
        error << ")";
        throw Exception(error.str().c_str());
      }
    }

    return {boost::get<0>(result), ExtractContent(responseTree),
            boost::get<2>(result)};
  }

 protected:
  virtual const ptr::ptree &ExtractContent(
      const ptr::ptree &responseTree) const {
    const auto &result = responseTree.get_child_optional("result");
    if (!result) {
      boost::format error(
          "The server did not return response to the request \"%1%\"");
      error % GetName();
      throw CommunicationError(error.str().c_str());
    }
    return *result;
  }
  virtual FloodControl &GetFloodControl() override { return m_floodControl; }

 private:
  FloodControl &m_floodControl;
};

class PublicRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PublicRequest(const std::string &name,
                         FloodControl &floodControl,
                         const std::string &uriParams,
                         const Context &context,
                         ModuleEventsLog &log)
      : Base("/t/api_pub.html", name, floodControl, uriParams, context, log) {}

 protected:
  virtual bool IsPriority() const { return false; }
};

class PrivateRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PrivateRequest(const std::string &name,
                          const Settings &settings,
                          FloodControl &floodControl,
                          bool isPriority,
                          const std::string &uriParams,
                          const Context &context,
                          ModuleEventsLog &log,
                          ModuleTradingLog *tradingLog = nullptr)
      : Base("/t/api.html",
             name,
             floodControl,
             AppendUriParams("apikey=" + settings.apiKey, uriParams),
             context,
             log,
             tradingLog),
        m_apiSecret(settings.apiSecret),
        m_isPriority(isPriority) {}

  virtual ~PrivateRequest() override = default;

 protected:
  virtual void PrepareRequest(const net::HTTPClientSession &session,
                              const std::string &body,
                              net::HTTPRequest &request) const override {
    using namespace trdk::Lib::Crypto;
    const auto &digest =
        Hmac::CalcSha512Digest((session.secure() ? "https://" : "http://") +
                                   session.getHost() + GetRequest().getURI(),
                               m_apiSecret);
    request.set("apisign", EncodeToHex(&digest[0], digest.size()));
    Base::PrepareRequest(session, body, request);
  }

  virtual bool IsPriority() const { return m_isPriority; }

 private:
  const std::string m_apiSecret;
  const bool m_isPriority;
};

std::string NormilizeProductId(const std::string &source) {
  return boost::replace_first_copy(source, "_", "-");
}
std::string RestoreSymbol(const std::string &source) {
  return boost::replace_first_copy(source, "-", "_");
}
}

////////////////////////////////////////////////////////////////////////////////

namespace {
class CcexExchange : public TradingSystem, public MarketDataSource {
 private:
  typedef boost::mutex SecuritiesMutex;
  typedef SecuritiesMutex::scoped_lock SecuritiesLock;

  struct SecuritySubscribtion {
    boost::shared_ptr<Rest::Security> security;
    boost::shared_ptr<Request> request;
    bool isSubscribed;
  };

 public:
  explicit CcexExchange(const App &,
                        const TradingMode &mode,
                        Context &context,
                        const std::string &instanceName,
                        const IniSectionRef &conf)
      : TradingSystem(mode, context, instanceName),
        MarketDataSource(context, instanceName),
        m_settings(conf, GetTsLog()),
        m_isConnected(false),
        m_endpoint(GetEndpoint()),
        m_marketDataSession(CreateSession(m_endpoint.host, m_settings, false)),
        m_tradingSession(CreateSession(m_endpoint.host, m_settings, true)),
        m_balances(*this, GetTsLog(), GetTsTradingLog()),
        m_pollingTask(boost::make_unique<PollingTask>(
            m_settings.pollingSetttings, GetMdsLog())) {}

  virtual ~CcexExchange() override {
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

  virtual Balances &GetBalancesStorage() override { return m_balances; }

  virtual Volume CalcCommission(const Volume &vol,
                                const trdk::Security &security) const override {
    return RoundByPrecision(vol * (0.2 / 100),
                            security.GetPricePrecisionPower());
  }

  virtual boost::optional<OrderCheckError> CheckOrder(
      const trdk::Security &security,
      const Currency &currency,
      const Qty &qty,
      const boost::optional<Price> &price,
      const OrderSide &side) const override {
    const auto &symbol = security.GetSymbol();
    if (symbol.GetQuoteSymbol() == "BTC") {
      if (price && qty * *price < 0.001) {
        return OrderCheckError{boost::none, boost::none, 0.001};
      }
      if (symbol.GetSymbol() == "ETH_BTC") {
        if (qty < 0.015) {
          return OrderCheckError{0.015};
        }
      }
    }
    return TradingSystem::CheckOrder(security, currency, qty, price, side);
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
        "Prices", 1,
        [this] {
          UpdateSecurities();
          return true;
        },
        m_settings.pollingSetttings.GetPricesRequestFrequency(), false);
    m_pollingTask->AddTask(
        "Actual orders", 0,
        [this] {
          UpdateOrders();
          return true;
        },
        m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
    m_pollingTask->AddTask(
        "Balances", 1,
        [this] {
          UpdateBalances();
          return true;
        },
        m_settings.pollingSetttings.GetBalancesRequestFrequency(), true);
    m_pollingTask->AddTask(
        "Opened orders", 100, [this] { return UpdateOpenedOrders(); },
        m_settings.pollingSetttings.GetAllOrdersRequestFrequency(), false);

    m_pollingTask->AccelerateNextPolling();

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
    result->SetTradingSessionState(pt::not_a_date_time, true);

    {
      const auto request = boost::make_shared<PublicRequest>(
          "getorderbook", m_endpoint.floodControl,
          "market=" + product->second.id + "&type=both&depth=1", GetContext(),
          GetTsLog());

      const SecuritiesLock lock(m_securitiesMutex);
      m_securities.emplace(symbol, SecuritySubscribtion{result, request})
          .first->second.security;
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

    boost::format requestParams("market=%1%&quantity=%2$.8f&rate=%3$.8f");
    requestParams % product->second.id  // 1
        % qty                           // 2
        % *price;                       // 3

    PrivateRequest request(side == ORDER_SIDE_SELL ? "selllimit" : "buylimit",
                           m_settings, m_endpoint.floodControl, true,
                           requestParams.str(), GetContext(), GetTsLog(),
                           &GetTsTradingLog());

    const auto &result = request.Send(*m_tradingSession);
    try {
      return boost::make_unique<OrderTransactionContext>(
          *this, boost::get<1>(result).get<OrderId>("uuid"));
    } catch (const std::exception &ex) {
      boost::format error("Failed to read order transaction reply: \"%1%\"");
      error % ex.what();
      throw Exception(error.str().c_str());
    }
  }

  virtual void SendCancelOrderTransaction(
      const OrderTransactionContext &transaction) override {
    const auto orderId = transaction.GetOrderId();

    class CancelOrderRequest : public PrivateRequest {
     public:
      typedef PrivateRequest Base;

     public:
      explicit CancelOrderRequest(const OrderId &orderId,
                                  const Settings &settings,
                                  FloodControl &floodControl,
                                  const Context &context,
                                  ModuleEventsLog &log,
                                  ModuleTradingLog &tradingLog)
          : Base("cancel",
                 settings,
                 floodControl,
                 true,
                 "uuid=" + boost::lexical_cast<std::string>(orderId),
                 context,
                 log,
                 &tradingLog) {}

      virtual ~CancelOrderRequest() override = default;

     protected:
      virtual const ptr::ptree &ExtractContent(
          const ptr::ptree &responseTree) const override {
        return responseTree;
      }
    } request(orderId, m_settings, m_endpoint.floodControl, GetContext(),
              GetTsLog(), GetTsTradingLog());
    request.Send(*m_tradingSession);

    //! @todo Bug here, move canceling to the update thread.
    GetContext().GetTimer().Schedule(
        [this, orderId]() {
          try {
            OnOrderCancel(GetContext().GetCurrentTime(), orderId);
          } catch (const OrderIsUnknown &) {
            if (m_orders.empty()) {
              throw;
            }
          }
        },
        m_timerScope);
  }

  virtual void OnTransactionSent(
      const OrderTransactionContext &transaction) override {
    TradingSystem::OnTransactionSent(transaction);
    m_pollingTask->AccelerateNextPolling();
  }

 private:
  void UpdateBalances() {
    PrivateRequest request("getbalances", m_settings, m_endpoint.floodControl,
                           false, std::string(), GetContext(), GetTsLog());
    ptr::ptree response;
    try {
      response = boost::get<1>(request.Send(*m_marketDataSession));
    } catch (const std::exception &ex) {
      boost::format error("Failed to request balance list: \"%1%\"");
      error % ex.what();
      throw CommunicationError(error.str().c_str());
    }
    for (const auto &node : response) {
      std::string symbol;
      Volume balance;
      Volume available;
      try {
        const auto &data = node.second;
        symbol = data.get<std::string>("Currency");
        balance = data.get<Volume>("Balance");
        available = data.get<Volume>("Available");
      } catch (const std::exception &ex) {
        boost::format error("Failed to read balance list: \"%1%\"");
        error % ex.what();
        throw CommunicationError(error.str().c_str());
      }
      AssertLe(available, balance);
      auto locked = balance - available;
      m_balances.Set(symbol, std::move(available), std::move(locked));
    }
  }

  void RequestProducts() {
    boost::unordered_map<std::string, Product> products;
    PublicRequest request("getmarkets", m_endpoint.floodControl, std::string(),
                          GetContext(), GetTsLog());
    try {
      const auto response = boost::get<1>(request.Send(*m_marketDataSession));
      for (const auto &node : response) {
        const auto &data = node.second;
        Product product = {data.get<std::string>("MarketName")};
        const auto &symbol = RestoreSymbol(product.id);
        const auto &productIt =
            products.emplace(std::move(symbol), std::move(product));
        if (!productIt.second) {
          GetTsLog().Error("Product duplicate: \"%1%\"",
                           productIt.first->first);
          Assert(productIt.second);
          continue;
        }
      }
    } catch (const std::exception &ex) {
      boost::format error("Failed to read supported product list: \"%1%\"");
      error % ex.what();
      throw Exception(error.str().c_str());
    }
    m_products = std::move(products);
  }

  void UpdateOrder(const CcexOrder &order, const OrderStatus &status) {
    const auto qtyPrecision = 1000000;
    OnOrder(order.id, order.symbol, status,
            RoundByPrecision(order.qty, qtyPrecision),
            RoundByPrecision(order.remainingQty, qtyPrecision), order.price,
            order.side, order.tif, order.openTime, order.updateTime);
  }

  CcexOrder UpdateOrder(const ptr::ptree &order, bool isActialOrder) {
    CcexOrder result;
    try {
      const auto &orderId = order.get<OrderId>("OrderUuid");
      const auto &type =
          order.get<std::string>(isActialOrder ? "Type" : "OrderType");

      OrderSide side;
      boost::optional<Price> price;
      if (type == "LIMIT_SELL") {
        side = ORDER_SIDE_SELL;
        price = order.get<double>("Limit");
      } else if (type == "LIMIT_BUY") {
        side = ORDER_SIDE_BUY;
        price = order.get<double>("Limit");
      } else {
        GetTsLog().Error("Unknown order type \"%1%\" for order %2%.", type,
                         orderId);
        throw Exception("Failed to request order status");
      }

      const Qty qty = order.get<double>("Quantity");
      const Qty remainingQty = order.get<double>("QuantityRemaining");

      const auto &openTime =
          pt::time_from_string(order.get<std::string>("Opened"));
      const auto &closedField = order.get<std::string>("Closed");
      pt::ptime updateTime;
      OrderStatus status;
      if (!boost::iequals(closedField, "null")) {
        updateTime = pt::time_from_string(closedField);
        status = remainingQty == qty ? ORDER_STATUS_CANCELED
                                     : ORDER_STATUS_FILLED_FULLY;
      } else {
        updateTime = openTime;
        status = order.get<bool>("CancelInitiated")
                     ? ORDER_STATUS_CANCELED
                     : remainingQty == qty ? ORDER_STATUS_OPENED
                                           : ORDER_STATUS_FILLED_PARTIALLY;
      }

      result = {std::move(orderId),
                order.get<std::string>("Exchange"),
                std::move(status),
                std::move(qty),
                std::move(remainingQty),
                std::move(price),
                std::move(side),
                order.get<bool>("ImmediateOrCancel") ? TIME_IN_FORCE_IOC
                                                     : TIME_IN_FORCE_GTC,
                std::move(openTime),
                std::move(updateTime)};
    } catch (const std::exception &ex) {
      boost::format error(
          "Failed to read state for order: \"%1%\" (response: \"%2%\")");
      error % ex.what()                     // 1
          % ConvertToString(order, false);  // 2
      throw CommunicationError(error.str().c_str());
    }

    UpdateOrder(result, result.status);
    return result;
  }

  bool UpdateOpenedOrders() {
    boost::unordered_map<OrderId, CcexOrder> notifiedOrderOrders;
    bool isInitial = m_orders.empty();
    {
      PrivateRequest request("getopenorders", m_settings,
                             m_endpoint.floodControl, false, std::string(),
                             GetContext(), GetTsLog(), &GetTsTradingLog());
      try {
        const auto orders = boost::get<1>(request.Send(*m_marketDataSession));
        for (const auto &order : orders) {
          const auto notifiedOrder = UpdateOrder(order.second, false);
          if (notifiedOrder.status != ORDER_STATUS_CANCELED &&
              (isInitial || m_orders.count(notifiedOrder.id))) {
            const auto id = notifiedOrder.id;
            notifiedOrderOrders.emplace(id, std::move(notifiedOrder));
          }
          m_orders.erase(notifiedOrder.id);
        }
      } catch (const std::exception &ex) {
        boost::format error("Failed to request order list: \"%1%\"");
        error % ex.what();
        throw CommunicationError(error.str().c_str());
      }
    }
    for (const auto &canceledOrder : m_orders) {
      UpdateOrder(canceledOrder.second, ORDER_STATUS_CANCELED);
    }
    m_orders.swap(notifiedOrderOrders);

    return !m_orders.empty();
  }

  void UpdateSecurities() {
    const SecuritiesLock lock(m_securitiesMutex);
    for (const auto &subscribtion : m_securities) {
      if (!subscribtion.second.isSubscribed) {
        continue;
      }
      auto &security = *subscribtion.second.security;
      auto &request = *subscribtion.second.request;
      try {
        const auto &response = request.Send(*m_marketDataSession);
        const auto &time = boost::get<0>(response);
        const auto &delayMeasurement = boost::get<2>(response);
        for (const auto &updateRecord :
             boost::get<1>(response).get_child("buy")) {
          const auto &update = updateRecord.second;
          security.SetLevel1(time,
                             Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
                                 update.get<double>("Rate")),
                             Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(
                                 update.get<double>("Quantity")),
                             delayMeasurement);
        }
        for (const auto &updateRecord :
             boost::get<1>(response).get_child("sell")) {
          const auto &update = updateRecord.second;
          security.SetLevel1(time,
                             Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
                                 update.get<double>("Rate")),
                             Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(
                                 update.get<double>("Quantity")),
                             delayMeasurement);
        }
      } catch (const std::exception &ex) {
        try {
          security.SetOnline(pt::not_a_date_time, false);
        } catch (...) {
          AssertFailNoException();
          throw;
        }
        boost::format error("Failed to read order book for \"%1%\": \"%2%\"");
        error % security  // 1
            % ex.what();  // 2
        throw CommunicationError(error.str().c_str());
      }
      security.SetOnline(pt::not_a_date_time, true);
    }
  }

  void UpdateOrders() {
    for (const OrderId &orderId : GetActiveOrderIdList()) {
      PrivateRequest request(
          "getorder", m_settings, m_endpoint.floodControl, false,
          "uuid=" + boost::lexical_cast<std::string>(orderId), GetContext(),
          GetTsLog(), &GetTsTradingLog());
      ptr::ptree response;
      try {
        response = boost::get<1>(request.Send(*m_marketDataSession));
      } catch (const OrderIsUnknown &) {
        OnOrderStatusUpdate(GetContext().GetCurrentTime(), orderId,
                            ORDER_STATUS_FILLED_FULLY, 0, Trade{});
      } catch (const std::exception &ex) {
        boost::format error(
            "Failed to request state for order %1%: \"%2%\" (request: \"%3%\", "
            "response: \"%4%\")");
        error % orderId                          // 1
            % ex.what()                          // 2
            % request.GetRequest().getURI()      // 3
            % ConvertToString(response, false);  // 4
        throw CommunicationError(error.str().c_str());
      }

      for (const auto &order : response) {
        UpdateOrder(order.second, true);
      }
    }
  }

 private:
  Settings m_settings;

  bool m_isConnected;
  Endpoint &m_endpoint;
  std::unique_ptr<Poco::Net::HTTPClientSession> m_marketDataSession;
  std::unique_ptr<Poco::Net::HTTPClientSession> m_tradingSession;

  boost::unordered_map<std::string, Product> m_products;

  SecuritiesMutex m_securitiesMutex;
  boost::unordered_map<Lib::Symbol, SecuritySubscribtion> m_securities;

  BalancesContainer m_balances;

  std::unique_ptr<PollingTask> m_pollingTask;

  boost::unordered_map<OrderId, CcexOrder> m_orders;

  trdk::Timer::Scope m_timerScope;
};
}

////////////////////////////////////////////////////////////////////////////////

TradingSystemAndMarketDataSourceFactoryResult CreateCcex(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<CcexExchange>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return {result, result};
}

boost::shared_ptr<MarketDataSource> CreateCcexMarketDataSource(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<CcexExchange>(App::GetInstance(), TRADING_MODE_LIVE,
                                          context, instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
