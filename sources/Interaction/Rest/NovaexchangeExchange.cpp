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
#include "Core/MarketDataSource.hpp"
#include "Core/TradingSystem.hpp"
#include "App.hpp"
#include "PollingTask.hpp"
#include "Request.hpp"
#include "Security.hpp"

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
}

////////////////////////////////////////////////////////////////////////////////

namespace {

class NovaexchangeRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit NovaexchangeRequest(const std::string &uri,
                               const std::string &message,
                               const std::string &method,
                               const Settings &settings)
      : Base(uri, message, method, settings.apiKey, settings.apiSecret) {}

  virtual ~NovaexchangeRequest() override = default;

 public:
  virtual boost::tuple<pt::ptime, ptr::ptree, Milestones> Send(
      net::HTTPSClientSession &session, const Context &context) override {
    auto result = Base::Send(session, context);
    auto &responseTree = boost::get<1>(result);
    CheckResponseError(responseTree);
    return boost::make_tuple(std::move(boost::get<0>(result)),
                             ExtractContent(responseTree),
                             std::move(boost::get<2>(result)));
  }

 protected:
  virtual const ptr::ptree &ExtractContent(const ptr::ptree &responseTree) {
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

class OpenOrdersNovaexchangeRequest : public NovaexchangeRequest {
 public:
  typedef NovaexchangeRequest Base;

 public:
  explicit OpenOrdersNovaexchangeRequest(const std::string &uri,
                                         const std::string &message,
                                         const std::string &method,
                                         const Settings &settings)
      : Base(uri, message, method, settings) {}

  virtual ~OpenOrdersNovaexchangeRequest() override = default;

 protected:
  virtual const ptr::ptree &ExtractContent(
      const ptr::ptree &responseTree) override {
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
    boost::shared_ptr<NovaexchangeRequest> request;
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
        m_session("novaexchange.com"),
        m_getBalancesRequest("/remote/v2/private/getbalances/",
                             "balances",
                             net::HTTPRequest::HTTP_POST,
                             m_settings) {
    m_session.setKeepAlive(true);
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
    GetMdsLog().Info("Creating connection...");
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
            const auto &response = request.Send(m_session, GetContext());
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
    try {
      const auto &response = m_getBalancesRequest.Send(m_session, GetContext());
      for (const auto &currency : boost::get<1>(response)) {
        const Volume totalAmount = currency.second.get<double>("amount_total");
        if (!totalAmount) {
          continue;
        }
        GetTsLog().Info(
            "%1% (%2%) balance: %3% (amount: %4%, trades: %5%, locked: %6%).",
            currency.second.get<std::string>("currencyname"),  // 1
            currency.second.get<std::string>("currency"),      // 2
            totalAmount,                                       // 3
            currency.second.get<double>("amount"),             // 4
            currency.second.get<double>("amount_trades"),      // 5
            currency.second.get<double>("amount_lockbox"));    // 6
      }
    } catch (const std::exception &ex) {
      boost::format error("Failed to read server balance list: \"%1%\"");
      error % ex.what();
      throw TradingSystem::ConnectError(error.str().c_str());
    }
    m_isConnected = true;
  }

  virtual trdk::Security &CreateNewSecurityObject(
      const Symbol &symbol) override {
    {
      const auto &it = m_securities.find(symbol);
      if (it != m_securities.cend()) {
        return *it->second.security;
      }
    }
    const SecuritiesLock lock(m_securitiesMutex);

    std::vector<std::string> subs;
    boost::split(subs, symbol.GetSymbol(), boost::is_any_of("_"));
    subs[0].swap(subs[1]);

    return *m_securities
                .emplace(
                    symbol,
                    SecuritySubscribtion{
                        boost::make_shared<Rest::Security>(
                            GetContext(), symbol, *this,
                            Rest::Security::SupportedLevel1Types()
                                .set(LEVEL1_TICK_BID_PRICE)
                                .set(LEVEL1_TICK_BID_QTY)
                                .set(LEVEL1_TICK_ASK_PRICE)
                                .set(LEVEL1_TICK_ASK_QTY)),
                        boost::make_shared<OpenOrdersNovaexchangeRequest>(
                            "/remote/v2/market/openorders/" +
                                boost::join(subs, "_") + "/BOTH/",
                            "orders", net::HTTPRequest::HTTP_POST, m_settings)})
                .first->second.security;
  }

  virtual OrderId SendSellAtMarketPrice(trdk::Security &,
                                        const Currency &,
                                        const Qty &,
                                        const OrderParams &) override {
    throw MethodIsNotImplementedException("Methods is not supported");
  }

  virtual OrderId SendSell(trdk::Security &,
                           const Currency &,
                           const Qty &,
                           const Price &,
                           const OrderParams &) override {
    throw MethodIsNotImplementedException("Methods is not supported");
  }

  virtual OrderId SendSellImmediatelyOrCancel(trdk::Security &,
                                              const Currency &,
                                              const Qty &,
                                              const Price &,
                                              const OrderParams &) override {
    throw MethodIsNotImplementedException("Methods is not supported");
  }

  virtual OrderId SendSellAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const Currency &,
      const Qty &,
      const OrderParams &) override {
    throw MethodIsNotImplementedException("Methods is not supported");
  }

  virtual OrderId SendBuyAtMarketPrice(trdk::Security &,
                                       const Currency &,
                                       const Qty &,
                                       const OrderParams &) override {
    throw MethodIsNotImplementedException("Methods is not supported");
  }

  virtual OrderId SendBuy(trdk::Security &,
                          const Currency &,
                          const Qty &,
                          const Price &,
                          const OrderParams &) override {
    throw MethodIsNotImplementedException("Methods is not supported");
  }

  virtual OrderId SendBuyImmediatelyOrCancel(trdk::Security &,
                                             const Currency &,
                                             const Qty &,
                                             const Price &,
                                             const OrderParams &) override {
    throw MethodIsNotImplementedException("Methods is not supported");
  }

  virtual OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const Currency &,
      const Qty &,
      const OrderParams &) override {
    throw MethodIsNotImplementedException("Methods is not supported");
  }

  virtual void SendCancelOrder(const OrderId &) override {
    throw MethodIsNotImplementedException("Methods is not supported");
  }

 private:
  Settings m_settings;

  bool m_isConnected;
  net::HTTPSClientSession m_session;
  NovaexchangeRequest m_getBalancesRequest;

  SecuritiesMutex m_securitiesMutex;
  boost::unordered_map<Symbol, SecuritySubscribtion> m_securities;

  std::unique_ptr<PollingTask> m_pollingTask;
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

////////////////////////////////////////////////////////////////////////////////
