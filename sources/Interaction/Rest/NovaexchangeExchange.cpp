/*******************************************************************************
 *   Created: 2017/10/09 16:57:59
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

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
    log.Info("API key: \"%1%\". API secret: %2%. Polling Interval: %3%.",
             apiKey,                                    // 1
             apiSecret.empty() ? "not set" : "is set",  // 2
             pollingInterval);                          // 3
  }

  void Validate() {}
};

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
void ReadTopOfBook(const pt::ptime &time,
                   const ptr::ptree &bids,
                   const ptr::ptree &asks,
                   Rest::Security &security,
                   const Milestones &delayMeasurement) {
  for (const auto &bidNode : bids) {
    const auto &bid = bidNode.second;
    for (const auto &askNode : asks) {
      const auto &ask = askNode.second;
      security.SetLevel1(time, Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
                                   bid.get<double>("price")),
                         Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(
                             bid.get<double>("amount")),
                         Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
                             ask.get<double>("price")),
                         Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(
                             ask.get<double>("amount")),
                         delayMeasurement);
      return;
    }
    security.SetLevel1(
        time, Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
                  bid.get<double>("price")),
        Level1TickValue::Create<LEVEL1_TICK_BID_QTY>(bid.get<double>("amount")),
        delayMeasurement);
    return;
  }
  for (const auto &askNode : asks) {
    const auto &ask = askNode.second;
    security.SetLevel1(
        time, Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
                  ask.get<double>("price")),
        Level1TickValue::Create<LEVEL1_TICK_ASK_QTY>(ask.get<double>("amount")),
        delayMeasurement);
    return;
  }
}
#pragma warning(pop)

std::string NormilizeSymbol(const std::string &source) {
  std::vector<std::string> subs;
  boost::split(subs, source, boost::is_any_of("_"));
  if (subs.size() != 2) {
    return source;
  }
  subs[0].swap(subs[1]);
  return boost::join(subs, "_");
}
}

////////////////////////////////////////////////////////////////////////////////

namespace {

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

 public:
  explicit Request(const std::string &uri,
                   const std::string &name,
                   const std::string &method,
                   const std::string &uriParams = std::string())
      : Base(uri, name, method, uriParams) {}

  virtual ~Request() override = default;

 public:
  virtual boost::tuple<pt::ptime, ptr::ptree, Milestones> Send(
      net::HTTPClientSession &session, const Context &context) override {
    auto result = Base::Send(session, context);
    auto &responseTree = boost::get<1>(result);
    CheckResponseError(responseTree);
    return boost::make_tuple(std::move(boost::get<0>(result)),
                             ExtractContent(responseTree),
                             std::move(boost::get<2>(result)));
  }

 protected:
  virtual const ptr::ptree &ExtractContent(
      const ptr::ptree &responseTree) const {
    const auto &result = responseTree.get_child_optional(GetName());
    if (!result) {
      boost::format error(
          "The server did not return response to the request \"%1%\"");
      error % GetName();
      throw Exception(error.str().c_str());
    }
    return *result;
  }

  void CheckResponseError(const ptr::ptree &responseTree) const {
    const auto &status = responseTree.get_optional<std::string>("status");
    if (!status || *status != "success") {
      const auto &message = responseTree.get_optional<std::string>("message");
      std::ostringstream error;
      error << "The server returned an error in response to the request \""
            << GetName() << "\" (" << GetRequest().getURI() << "): ";
      if (message) {
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
};

class PublicRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PublicRequest(const std::string &uri,
                         const std::string &name,
                         const std::string &uriParams = std::string())
      : Base(uri, name, net::HTTPRequest::HTTP_GET, uriParams) {}
};

class PrivateRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PrivateRequest(const std::string &uri,
                          const std::string &name,
                          const Settings &settings,
                          const std::string &uriParams = std::string())
      : Base(uri,
             name,
             net::HTTPRequest::HTTP_POST,
             AppendUriParams("apikey=" + settings.apiKey, uriParams)),
        m_apiSecret(settings.apiSecret) {}

  virtual ~PrivateRequest() override = default;

 protected:
  virtual void CreateBody(const net::HTTPClientSession &session,
                          std::string &result) const override {
    if (!result.empty()) {
      result += '&';
    }
    result += "signature=";
    {
      using namespace trdk::Lib::Crypto;
      const auto &digest =
          CalcHmacSha512Digest((session.secure() ? "https://" : "http://") +
                                   session.getHost() + GetRequest().getURI(),
                               m_apiSecret);
      result += Base64Coder(false).Encode(&digest[0], digest.size());
    }
    Base::CreateBody(session, result);
  }

 private:
  const std::string m_apiSecret;
};

class OpenOrdersRequest : public PublicRequest {
 public:
  typedef PublicRequest Base;

 public:
  explicit OpenOrdersRequest(const std::string &uri) : Base(uri, "orders") {}

  virtual ~OpenOrdersRequest() override = default;

 protected:
  virtual const ptr::ptree &ExtractContent(
      const ptr::ptree &responseTree) const override {
    return responseTree;
  }
};
}

////////////////////////////////////////////////////////////////////////////////

namespace {
class NovaexchangeExchange : public TradingSystem, public MarketDataSource {
 private:
  typedef boost::mutex SecuritiesMutex;
  typedef SecuritiesMutex::scoped_lock SecuritiesLock;

  struct SecuritySubscribtion {
    boost::shared_ptr<Rest::Security> security;
    boost::shared_ptr<Request> request;
    bool isSubscribed;
  };

 public:
  explicit NovaexchangeExchange(const App &,
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
        m_marketDataSession("novaexchange.com"),
        m_tradingSession(m_marketDataSession.getHost()) {
    m_marketDataSession.setKeepAlive(true);
    m_tradingSession.setKeepAlive(true);
  }

  virtual ~NovaexchangeExchange() override = default;

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
          for (const auto &subscribtion : m_securities) {
            if (!subscribtion.second.isSubscribed) {
              continue;
            }
            auto &security = *subscribtion.second.security;
            auto &request = *subscribtion.second.request;
            const auto &response =
                request.Send(m_marketDataSession, GetContext());
            const auto &time = boost::get<0>(response);
            const auto &delayMeasurement = boost::get<2>(response);
            const auto &update = boost::get<1>(response);
            try {
              ReadTopOfBook(time, update.get_child("buyorders"),
                            update.get_child("sellorders"), security,
                            delayMeasurement);
            } catch (const std::exception &ex) {
              boost::format error(
                  "Failed to read order book for \"%1%\": \"%2%\"");
              error % security % ex.what();
              throw MarketDataSource::Error(error.str().c_str());
            }
          }
        },
        m_settings.pollingInterval, GetMdsLog());
  }

 protected:
  virtual void CreateConnection(const IniSectionRef &) override {
    PrivateRequest request("/remote/v2/private/getbalances/", "balances",
                           m_settings);
    try {
      const auto &response = request.Send(m_marketDataSession, GetContext());
      MakeServerAnswerDebugDump(boost::get<1>(response), *this);
      for (const auto &currency : boost::get<1>(response)) {
        const Volume totalAmount = currency.second.get<double>("amount_total");
        if (!totalAmount) {
          continue;
        }
        GetTsLog().Info(
            "%1% (%2%) balance: %3% (amount: %4%, trades: %5%, locked: "
            "%6%).",
            currency.second.get<std::string>("currencyname"),  // 1
            currency.second.get<std::string>("currency"),      // 2
            totalAmount,                                       // 3
            currency.second.get<double>("amount"),             // 4
            currency.second.get<double>("amount_trades"),      // 5
            currency.second.get<double>("amount_lockbox"));    // 6
      }
    } catch (const std::exception &ex) {
      GetTsLog().Error("Failed to read server balance list: \"%1%\".",
                       ex.what());
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
      const auto marketDataRequest = boost::make_shared<OpenOrdersRequest>(
          "/remote/v2/market/openorders/" +
          NormilizeSymbol(result->GetSymbol().GetSymbol()) + "/BOTH/");

      const SecuritiesLock lock(m_securitiesMutex);
      Verify(m_securities
                 .emplace(
                     symbol,
                     SecuritySubscribtion{result, std::move(marketDataRequest)})
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

    boost::format requestParams(
        "tradetype=BUY&tradeamount=%1$.8f&tradeprice=%2$.8f&tradebase=0");
    requestParams % qty  // 1
        % *price;        // 2
    PrivateRequest request(
        "/remote/v2/private/trade/" +
            NormilizeSymbol(security.GetSymbol().GetSymbol()) + "/",
        "tradeitems", m_settings, requestParams.str());
    const auto &result = request.Send(m_tradingSession, GetContext());
    MakeServerAnswerDebugDump(boost::get<1>(result), *this);

    boost::optional<OrderId> orderId;
    std::vector<TradeInfo> trades;
    for (const auto &node : boost::get<1>(result)) {
      const auto &item = node.second;
      try {
        const auto &type = item.get<std::string>("type");
        if (boost::iequals(type, "trade")) {
          trades.emplace_back(TradeInfo{
              item.get<double>("price"),
              item.get<double>("fromamount") - item.get<double>("toamount")});
        } else if (boost::iequals(type, "created")) {
          Assert(!orderId);
          orderId = item.get<OrderId>("orderid");
          GetContext().GetTimer().Schedule(
              [this, orderId, qty] {
                OnOrderStatusUpdate(*orderId, ORDER_STATUS_SUBMITTED, qty);
              },
              m_timerScope);
        }
      } catch (const std::exception &ex) {
        boost::format error("Failed to read order transaction reply: \"%1%\"");
        error % ex.what();
        throw TradingSystem::Error(error.str().c_str());
      }
    }

    if (!orderId) {
      throw TradingSystem::Error("Exchange answer is empty");
    }

    if (!trades.empty()) {
      GetContext().GetTimer().Schedule(
          boost::bind(&NovaexchangeExchange::OnTradesInfo, this, *orderId, qty,
                      trades),
          m_timerScope);
    }
    return boost::make_unique<OrderTransactionContext>(std::move(*orderId));
  }

  virtual void SendCancelOrderTransaction(const OrderId &orderId) override {
    PrivateRequest request("/remote/v2/private/cancelorder/" +
                               boost::lexical_cast<std::string>(orderId) + "/",
                           "cancelorder", m_settings);
    const auto &result = request.Send(m_tradingSession, GetContext());
    MakeServerAnswerDebugDump(boost::get<1>(result), *this);
  }

  void OnTradesInfo(const OrderId &orderId,
                    Qty &remainingQty,
                    std::vector<TradeInfo> &trades) {
    for (auto &trade : trades) {
      remainingQty -= trade.qty;
      OnOrderStatusUpdate(orderId,
                          remainingQty > 0 ? ORDER_STATUS_FILLED_PARTIALLY
                                           : ORDER_STATUS_FILLED,
                          remainingQty, std::move(trade));
    }
  }

 private:
  Settings m_settings;

  bool m_isConnected;
  net::HTTPSClientSession m_marketDataSession;
  net::HTTPSClientSession m_tradingSession;

  SecuritiesMutex m_securitiesMutex;
  boost::unordered_map<Symbol, SecuritySubscribtion> m_securities;

  std::unique_ptr<PollingTask> m_pollingTask;
  trdk::Timer::Scope m_timerScope;
};
}

////////////////////////////////////////////////////////////////////////////////

TradingSystemAndMarketDataSourceFactoryResult CreateNovaexchange(
    const TradingMode &mode,
    size_t tradingSystemIndex,
    size_t marketDataSourceIndex,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<NovaexchangeExchange>(
      App::GetInstance(), mode, tradingSystemIndex, marketDataSourceIndex,
      context, instanceName, configuration);
  return {result, result};
}

boost::shared_ptr<MarketDataSource> CreateNovaexchangeMarketDataSource(
    size_t index,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<NovaexchangeExchange>(
      App::GetInstance(), TRADING_MODE_LIVE, index, index, context,
      instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
