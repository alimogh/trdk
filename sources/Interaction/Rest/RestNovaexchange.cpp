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
#include "RestApp.hpp"
#include "RestPollingTask.hpp"
#include "RestRequest.hpp"
#include "RestSecurity.hpp"

using namespace trdk;
using namespace trdk::Lib;
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
  virtual boost::tuple<boost::posix_time::ptime,
                       boost::property_tree::ptree,
                       Lib::TimeMeasurement::Milestones>
  Send(net::HTTPSClientSession &session, const Context &context) override {
    auto result = Base::Send(session, context);
    auto &responseTree = boost::get<1>(result);

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

    const auto &contentTree = responseTree.get_child_optional(GetName());
    if (!contentTree) {
      boost::format error(
          "The server did not return response to the request \"%1%\"");
      error % GetName();
      throw Exception(error.str().c_str());
    }

    return boost::make_tuple(std::move(boost::get<0>(result)),
                             std::move(*contentTree),
                             std::move(boost::get<2>(result)));
  }
};
}

////////////////////////////////////////////////////////////////////////////////

namespace {
class Novaexchange : public TradingSystem, public MarketDataSource {
 public:
  explicit Novaexchange(const App &,
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

  virtual ~Novaexchange() override = default;

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
    m_pollingTask = boost::make_unique<PollingTask>(
        [this]() {
          for (auto &security : m_securities) {
            const auto &response =
                security.second->Send(m_session, GetContext());
            const auto &time = boost::get<0>(response);
            const auto &delayMeasurement = boost::get<2>(response);
            try {
              for (const auto &updateRecord : boost::get<1>(response)) {
                const auto &update = updateRecord.second;
                // const auto &marketName =
                // update.get<std::string>("marketname");
                //                 if (marketName !=
                //                 security.first->GetSymbol().GetSymbol()) {
                //                   boost::format error(
                //                       "Received update by wrong currency pair
                //                       \"%1%\" for "
                //                       "\"%2%\"");
                //                   error % marketName      // 1
                //                       % *security.first;  // 2
                //                   throw
                //                   MarketDataSource::Error(error.str().c_str());
                //                 }
                security.first->SetLevel1(
                    time, Level1TickValue::Create<LEVEL1_TICK_BID_PRICE>(
                              update.get<double>("bid")),
                    Level1TickValue::Create<LEVEL1_TICK_ASK_PRICE>(
                        update.get<double>("ask")),
                    Level1TickValue::Create<LEVEL1_TICK_LAST_PRICE>(
                        update.get<double>("last_price")),
                    delayMeasurement);
              }
            } catch (const std::exception &ex) {
              boost::format error(
                  "Failed to read market state for \"%1%\": \"%2%\"");
              error % *security.first % ex.what();
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
    std::vector<std::string> subs;
    boost::split(subs, symbol.GetSymbol(), boost::is_any_of("_"));
    subs[0].swap(subs[1]);

    m_securities.emplace_back(std::make_pair(
        boost::make_shared<Rest::Security>(
            GetContext(), symbol, *this,
            Rest::Security::SupportedLevel1Types()
                .set(LEVEL1_TICK_BID_PRICE)
                .set(LEVEL1_TICK_ASK_PRICE)),
        boost::make_shared<NovaexchangeRequest>(
            "/remote/v2/market/info/" + boost::join(subs, "_") + "/", "markets",
            net::HTTPRequest::HTTP_POST, m_settings)));
    return *m_securities.back().first;
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

  std::vector<std::pair<boost::shared_ptr<Rest::Security>,
                        boost::shared_ptr<NovaexchangeRequest>>>
      m_securities;

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
  const auto &result = boost::make_shared<Novaexchange>(
      App::GetInstance(), mode, tradingSystemIndex, marketDataSourceIndex,
      context, instanceName, configuration);
  return {result, result};
}

////////////////////////////////////////////////////////////////////////////////
