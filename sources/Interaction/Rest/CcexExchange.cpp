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
#include "App.hpp"
#include "FloodControl.hpp"
#include "PullingTask.hpp"
#include "Request.hpp"
#include "Security.hpp"
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

struct Settings {
  std::string apiKey;
  std::string apiSecret;
  pt::time_duration pullingInterval;

  explicit Settings(const IniSectionRef &conf, ModuleEventsLog &log)
      : apiKey(conf.ReadKey("api_key")),
        apiSecret(conf.ReadKey("api_secret")),
        pullingInterval(pt::milliseconds(
            conf.ReadTypedKey<long>("pulling_interval_milliseconds"))) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) {
    log.Info("API key: \"%1%\". API secret: %2%. Pulling interval: %3%.",
             apiKey,                                    // 1
             apiSecret.empty() ? "not set" : "is set",  // 2
             pullingInterval);                          // 3
  }

  void Validate() {}
};

struct Order {
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
                   const std::string &uriParams = std::string())
      : Base(uri,
             name,
             net::HTTPRequest::HTTP_GET,
             AppendUriParams("a=" + name, uriParams)),
        m_floodControl(floodControl) {}

  virtual ~Request() override = default;

 public:
  virtual boost::tuple<pt::ptime, ptr::ptree, Milestones> Send(
      net::HTTPClientSession &session, const Context &context) override {
    auto result = Base::Send(session, context);
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
            throw TradingSystem::OrderIsUnknown(message->c_str());
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
      throw Exception(error.str().c_str());
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
                         const std::string &uriParams = std::string())
      : Base("/t/api_pub.html", name, floodControl, uriParams) {}

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
                          const std::string &uriParams = std::string())
      : Base("/t/api.html",
             name,
             floodControl,
             AppendUriParams("apikey=" + settings.apiKey, uriParams)),
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

std::string NormilizeSymbol(const std::string &source) {
  return boost::replace_first_copy(source, "_", "-");
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
                        size_t tradingSystemIndex,
                        size_t marketDataSourceIndex,
                        Context &context,
                        const std::string &instanceName,
                        const IniSectionRef &conf)
      : TradingSystem(mode, tradingSystemIndex, context, instanceName),
        MarketDataSource(marketDataSourceIndex, context, instanceName),
        m_settings(conf, GetTsLog()),
        m_isConnected(false),
        m_endpoint(GetEndpoint()),
        m_marketDataSession(m_endpoint.host),
        m_tradingSession(m_endpoint.host),
        m_pullingTask(m_settings.pullingInterval, GetMdsLog()) {
    m_marketDataSession.setKeepAlive(true);
    m_tradingSession.setKeepAlive(true);
  }

  virtual ~CcexExchange() override = default;

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

 protected:
  virtual void CreateConnection(const IniSectionRef &) override {
    Verify(m_pullingTask.AddTask("Securities", 1,
                                 [this] {
                                   UpdateSecurities();
                                   return true;
                                 },
                                 1));
    Verify(m_pullingTask.AddTask("Actual orders", 0,
                                 [this] {
                                   UpdateOrders();
                                   return true;
                                 },
                                 2));
    Verify(m_pullingTask.AddTask("Opened orders", 100,
                                 [this] { return RequestOpenedOrders(); }, 30));
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
    result->SetTradingSessionState(pt::not_a_date_time, true);

    {
      const auto request = boost::make_shared<PublicRequest>(
          "getorderbook", m_endpoint.floodControl,
          "market=" + NormilizeSymbol(result->GetSymbol().GetSymbol()) +
              "&type=both&depth=1");

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
        throw TradingSystem::Error("Order time-in-force type is not supported");
    }
    if (currency != security.GetSymbol().GetCurrency()) {
      throw TradingSystem::Error(
          "Trading system supports only security quote currency");
    }
    if (!price) {
      throw TradingSystem::Error("Market order is not supported");
    }

    boost::format requestParams("market=%1%&quantity=%2$.8f&rate=%3$.8f");
    requestParams % NormilizeSymbol(security.GetSymbol().GetSymbol())  // 1
        % qty                                                          // 2
        % *price;                                                      // 3

    PrivateRequest request(side == ORDER_SIDE_SELL ? "selllimit" : "buylimit",
                           m_settings, m_endpoint.floodControl, true,
                           requestParams.str());

    const auto &result = request.Send(m_tradingSession, GetContext());
    try {
      return boost::make_unique<OrderTransactionContext>(
          boost::get<1>(result).get<OrderId>("uuid"));
    } catch (const std::exception &ex) {
      boost::format error("Failed to read order transaction reply: \"%1%\"");
      error % ex.what();
      throw TradingSystem::Error(error.str().c_str());
    }
  }

  virtual void SendCancelOrderTransaction(const OrderId &orderId) override {
    class CancelOrderRequest : public PrivateRequest {
     public:
      typedef PrivateRequest Base;

     public:
      explicit CancelOrderRequest(const OrderId &orderId,
                                  const Settings &settings,
                                  FloodControl &floodControl)
          : Base("cancel",
                 settings,
                 floodControl,
                 true,
                 "uuid=" + boost::lexical_cast<std::string>(orderId)) {}

      virtual ~CancelOrderRequest() override = default;

     protected:
      virtual const ptr::ptree &ExtractContent(
          const ptr::ptree &responseTree) const override {
        return responseTree;
      }
    } request(orderId, m_settings, m_endpoint.floodControl);
    request.Send(m_tradingSession, GetContext());

    GetContext().GetTimer().Schedule(
        [this, orderId]() {
          try {
            OnOrderCancel(orderId);
          } catch (const OrderIsUnknown &) {
            if (m_orders.empty()) {
              throw;
            }
          }
        },
        m_timerScope);
  }

  virtual void OnTransactionSent(const OrderId &orderId) override {
    TradingSystem::OnTransactionSent(orderId);
    m_pullingTask.AccelerateNextPulling();
  }

 private:
  void UpdateOrder(const Order &order, const OrderStatus &status) {
    const auto qtyPrecision = 1000000;
    OnOrder(order.id, order.symbol, status,
            RoundByPrecision(order.qty, qtyPrecision),
            RoundByPrecision(order.remainingQty, qtyPrecision), order.price,
            order.side, order.tif, order.openTime, order.updateTime);
  }

  Order UpdateOrder(const ptr::ptree &order, bool isActialOrder) {
    const auto &orderId = order.get<OrderId>("OrderUuid");

    OrderSide side;
    boost::optional<Price> price;
    const auto &type =
        order.get<std::string>(isActialOrder ? "Type" : "OrderType");
    if (type == "LIMIT_SELL") {
      side = ORDER_SIDE_SELL;
      price = order.get<double>("Limit");
    } else if (type == "LIMIT_BUY") {
      side = ORDER_SIDE_BUY;
      price = order.get<double>("Limit");
    } else {
      GetTsLog().Error("Unknown order type \"%1%\" for order %2%.", type,
                       orderId);
      throw TradingSystem::Error("Failed to request order status");
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
      status =
          remainingQty == qty ? ORDER_STATUS_CANCELLED : ORDER_STATUS_FILLED;
    } else {
      updateTime = openTime;
      status = order.get<bool>("CancelInitiated")
                   ? ORDER_STATUS_CANCELLED
                   : remainingQty == qty ? ORDER_STATUS_SUBMITTED
                                         : ORDER_STATUS_FILLED_PARTIALLY;
    }

    const Order result = {std::move(orderId),
                          order.get<std::string>("Exchange"),
                          std::move(status),
                          std::move(qty),
                          std::move(remainingQty),
                          std::move(price),
                          std::move(side),
                          order.get<bool>("ImmediateOrCancel")
                              ? TIME_IN_FORCE_IOC
                              : TIME_IN_FORCE_GTC,
                          std::move(openTime),
                          std::move(updateTime)};
    UpdateOrder(result, result.status);
    return result;
  }

  bool RequestOpenedOrders() {
    boost::unordered_map<OrderId, Order> notifiedOrderOrders;
    bool isInitial = m_orders.empty();
    {
      PrivateRequest request("getopenorders", m_settings,
                             m_endpoint.floodControl, false);
      try {
        const auto orders =
            boost::get<1>(request.Send(m_marketDataSession, GetContext()));
        for (const auto &order : orders) {
          const Order notifiedOrder = UpdateOrder(order.second, false);
          if (notifiedOrder.status != ORDER_STATUS_CANCELLED &&
              (isInitial || m_orders.count(notifiedOrder.id))) {
            const auto id = notifiedOrder.id;
            notifiedOrderOrders.emplace(id, std::move(notifiedOrder));
          }
          m_orders.erase(notifiedOrder.id);
        }
      } catch (const std::exception &ex) {
        GetTsLog().Error("Failed to request order list: \"%1%\".", ex.what());
        throw Exception("Failed to request order list");
      }
    }
    for (const auto &canceledOrder : m_orders) {
      UpdateOrder(canceledOrder.second, ORDER_STATUS_CANCELLED);
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
        const auto &response = request.Send(m_marketDataSession, GetContext());
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
        throw MarketDataSource::Error(error.str().c_str());
      }
      security.SetOnline(pt::not_a_date_time, true);
    }
  }

  void UpdateOrders() {
    for (const OrderId &orderId : GetActiveOrderList()) {
      PrivateRequest request(
          "getorder", m_settings, m_endpoint.floodControl, false,
          "uuid=" + boost::lexical_cast<std::string>(orderId));
      ptr::ptree response;
      try {
        response =
            boost::get<1>(request.Send(m_marketDataSession, GetContext()));
        for (const auto &order : response) {
          UpdateOrder(order.second, true);
        }
      } catch (const OrderIsUnknown &) {
        OnOrderStatusUpdate(orderId, ORDER_STATUS_FILLED, 0, {});
      } catch (const std::exception &ex) {
        GetTsLog().Error(
            "Failed to request state for order %1%: \"%2%\" (request: \"%3%\", "
            "response: \"%4%\").",
            orderId,                            // 1
            ex.what(),                          // 2
            request.GetRequest().getURI(),      // 3
            ConvertToString(response, false));  // 4
        throw Exception("Failed to request order state");
      }
    }
  }

 private:
  Settings m_settings;

  bool m_isConnected;
  Endpoint &m_endpoint;
  net::HTTPSClientSession m_marketDataSession;
  net::HTTPSClientSession m_tradingSession;

  SecuritiesMutex m_securitiesMutex;
  boost::unordered_map<Lib::Symbol, SecuritySubscribtion> m_securities;

  PullingTask m_pullingTask;

  boost::unordered_map<OrderId, Order> m_orders;

  trdk::Timer::Scope m_timerScope;
};
}

////////////////////////////////////////////////////////////////////////////////

TradingSystemAndMarketDataSourceFactoryResult CreateCcex(
    const TradingMode &mode,
    size_t tradingSystemIndex,
    size_t marketDataSourceIndex,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<CcexExchange>(
      App::GetInstance(), mode, tradingSystemIndex, marketDataSourceIndex,
      context, instanceName, configuration);
  return {result, result};
}

boost::shared_ptr<MarketDataSource> CreateCcexMarketDataSource(
    size_t index,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<CcexExchange>(App::GetInstance(), TRADING_MODE_LIVE,
                                          index, index, context, instanceName,
                                          configuration);
}

////////////////////////////////////////////////////////////////////////////////