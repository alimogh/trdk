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
#include "PollingTask.hpp"
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

struct Settings {
  std::string apiKey;
  std::string apiSecret;
  pt::time_duration pollingInterval;

  explicit Settings(const IniSectionRef &conf, ModuleEventsLog &log)
      : apiKey(conf.ReadKey("api_key")),
        apiSecret(conf.ReadKey("api_secret")),
        pollingInterval(pt::milliseconds(
            conf.ReadTypedKey<long>("polling_interval_milliseconds"))) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) {
    log.Info("API key: \"%1%\". API secret: %2%. Polling interval: %3%.",
             apiKey,                                    // 1
             apiSecret.empty() ? "not set" : "is set",  // 2
             pollingInterval);                          // 3
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
                   const std::string &uriParams = std::string())
      : Base(uri,
             name,
             net::HTTPRequest::HTTP_GET,
             AppendUriParams("a=" + name, uriParams)) {}

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

    return boost::make_tuple(std::move(boost::get<0>(result)),
                             std::move(ExtractContent(responseTree)),
                             std::move(boost::get<2>(result)));
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
};

class PublicRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PublicRequest(const std::string &name,
                         const std::string &uriParams = std::string())
      : Base("/t/api_pub.html", name, uriParams) {}
};

class PrivateRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PrivateRequest(const std::string &name,
                          const Settings &settings,
                          const std::string &uriParams = std::string())
      : Base("/t/api.html",
             name,
             AppendUriParams("apikey=" + settings.apiKey, uriParams)),
        m_apiSecret(settings.apiSecret) {}

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

 private:
  const std::string m_apiSecret;
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
        m_marketDataSession("c-cex.com"),
        m_tradingSession(m_marketDataSession.getHost()),
        m_numberOfPulls(0) {
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
          UpdateSecurities();
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
          "getorderbook",
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
                           m_settings, requestParams.str());

    const auto &result = request.Send(m_tradingSession, GetContext());
    MakeServerAnswerDebugDump(boost::get<1>(result), *this);

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
                                  const Settings &settings)
          : Base("cancel",
                 settings,
                 "uuid=" + boost::lexical_cast<std::string>(orderId)) {}

      virtual ~CancelOrderRequest() override = default;

     protected:
      virtual const ptr::ptree &ExtractContent(
          const ptr::ptree &responseTree) const override {
        return responseTree;
      }
    };

    try {
      const auto &result = CancelOrderRequest(orderId, m_settings)
                               .Send(m_tradingSession, GetContext());
      MakeServerAnswerDebugDump(boost::get<1>(result), *this);
    } catch (const OrderIsUnknown &) {
      throw;
    }
  }

 private:
  void UpdateOrder(const Order &order, const OrderStatus &status) {
    OnOrder(order.id, order.symbol, status, order.qty, order.remainingQty,
            order.price, order.side, order.tif, order.openTime,
            order.updateTime);
  }

  Order UpdateOrder(const ptr::ptree &order) {
    const auto &orderId = order.get<OrderId>("OrderUuid");

    OrderSide side;
    boost::optional<Price> price;
    const auto &type = order.get<std::string>("OrderType");
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

  void RequestOpenedOrders() {
    boost::unordered_map<OrderId, Order> notifiedOrderOrders;
    {
      PrivateRequest request("getopenorders", m_settings);
      try {
        const auto orders =
            boost::get<1>(request.Send(m_marketDataSession, GetContext()));
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
    }
    for (const auto &canceledOrder : m_orders) {
      UpdateOrder(canceledOrder.second, ORDER_STATUS_CANCELLED);
    }
    m_orders.swap(notifiedOrderOrders);
  }

  void UpdateSecurities() {
    const SecuritiesLock lock(m_securitiesMutex);
    for (const auto &subscribtion : m_securities) {
      if (!subscribtion.second.isSubscribed) {
        continue;
      }
      auto &security = *subscribtion.second.security;
      auto &request = *subscribtion.second.request;
      const auto &response = request.Send(m_marketDataSession, GetContext());
      const auto &time = boost::get<0>(response);
      const auto &delayMeasurement = boost::get<2>(response);
      try {
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
        boost::format error(
            "Failed to read order book state for \"%1%\": \"%2%\"");
        error % security  // 1
            % ex.what();  // 2
        throw MarketDataSource::Error(error.str().c_str());
      }
    }
  }

  void UpdateOrders() {
    for (const OrderId &orderId : GetActiveOrderList()) {
      PrivateRequest request(
          "getorder", m_settings,
          "uuid=" + boost::lexical_cast<std::string>(orderId));
      try {
        UpdateOrder(
            boost::get<1>(request.Send(m_marketDataSession, GetContext())));
      } catch (const OrderIsUnknown &ex) {
        GetTsLog().Warn("Failed to request state for order %1%: \"%2%\".",
                        orderId, ex.what());
        OnOrderCancel(orderId);
      } catch (const std::exception &ex) {
        GetTsLog().Error("Failed to request state for order %1%: \"%2%\".",
                         orderId, ex.what());
        throw Exception("Failed to request order state");
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

  boost::unordered_map<OrderId, Order> m_orders;

  size_t m_numberOfPulls;
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
