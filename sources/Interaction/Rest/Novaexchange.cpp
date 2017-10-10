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

class Novaexchange : public TradingSystem, public MarketDataSource {
 public:
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
        m_session("novaexchange.com"),
        m_getBalancesRequest("/private/getbalances/",
                             "balances",
                             net::HTTPRequest::HTTP_POST,
                             m_settings.apiKey,
                             m_settings.apiSecret) {
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
  virtual bool IsConnected() const override {
    return m_pollingTask ? true : false;
  }

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
          for (const auto &security : m_securities) {
            const auto &response =
                security->GetStateRequest().Send(m_session, GetContext());
            const auto &time = boost::get<0>(response);
            const auto &delayMeasurement = boost::get<2>(response);
            try {
              for (const auto &updateRecord : boost::get<1>(response)) {
                const auto &update = updateRecord.second;
                const auto &marketName = update.get<std::string>("marketname");
                if (marketName != security->GetSymbol().GetSymbol()) {
                  boost::format error(
                      "Received update by wrong currency pair \"%1%\" for "
                      "\"%2%\"");
                  error % marketName  // 1
                      % *security;    // 2
                  throw MarketDataSource::Error(error.str().c_str());
                }
                security->SetLevel1(
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
              error % *security % ex.what();
              throw MarketDataSource::Error(error.str().c_str());
            }
          }
        },
        m_settings.pollingInterval, GetMdsLog());
  }

 protected:
  virtual void CreateConnection(const IniSectionRef &) override {
    //     try {
    //       const auto &response = m_getBalancesRequest.Send(m_session,
    //       GetContext());
    //       for (const auto &currency : boost::get<1>(response)) {
    //         const Volume totalAmount =
    //         currency.second.get<double>("amount_total");
    //         if (!totalAmount) {
    //           continue;
    //         }
    //         GetTsLog().Info(
    //             "%1% (%2%) balance: %3% (amount: %4%, trades: %5%, locked:
    //             %6%).",
    //             currency.second.get<std::string>("currencyname"),  // 1
    //             currency.second.get<std::string>("currency"),      // 2
    //             totalAmount,                                       // 3
    //             currency.second.get<double>("amount"),             // 4
    //             currency.second.get<double>("amount_trades"),      // 5
    //             currency.second.get<double>("amount_lockbox"));    // 6
    //       }
    //     } catch (const std::exception &ex) {
    //       boost::format error("Failed to read server balance list: \"%1%\"");
    //       error % ex.what();
    //       throw TradingSystem::ConnectError(error.str().c_str());
    //     }
  }

  virtual trdk::Security &CreateNewSecurityObject(
      const Symbol &symbol) override {
    m_securities.emplace_back(boost::make_shared<Rest::Security>(
        GetContext(), symbol, *this,
        Request("/market/info/" + symbol.GetSymbol() + "/", "markets",
                net::HTTPRequest::HTTP_POST, m_settings.apiKey,
                m_settings.apiSecret),
        Rest::Security::SupportedLevel1Types()
            .set(LEVEL1_TICK_BID_PRICE)
            .set(LEVEL1_TICK_ASK_PRICE)));
    return *m_securities.back();
  }

  virtual OrderId SendSellAtMarketPrice(trdk::Security &,
                                        const Currency &,
                                        const Qty &,
                                        const OrderParams &) override {
    throw MethodDoesNotImplementedError("Methods is not supported");
  }

  virtual OrderId SendSell(trdk::Security &,
                           const Currency &,
                           const Qty &,
                           const Price &,
                           const OrderParams &) override {
    throw MethodDoesNotImplementedError("Methods is not supported");
  }

  virtual OrderId SendSellImmediatelyOrCancel(trdk::Security &,
                                              const Currency &,
                                              const Qty &,
                                              const Price &,
                                              const OrderParams &) override {
    throw MethodDoesNotImplementedError("Methods is not supported");
  }

  virtual OrderId SendSellAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const Currency &,
      const Qty &,
      const OrderParams &) override {
    throw MethodDoesNotImplementedError("Methods is not supported");
  }

  virtual OrderId SendBuyAtMarketPrice(trdk::Security &,
                                       const Currency &,
                                       const Qty &,
                                       const OrderParams &) override {
    throw MethodDoesNotImplementedError("Methods is not supported");
  }

  virtual OrderId SendBuy(trdk::Security &,
                          const Currency &,
                          const Qty &,
                          const Price &,
                          const OrderParams &) override {
    throw MethodDoesNotImplementedError("Methods is not supported");
  }

  virtual OrderId SendBuyImmediatelyOrCancel(trdk::Security &,
                                             const Currency &,
                                             const Qty &,
                                             const Price &,
                                             const OrderParams &) override {
    throw MethodDoesNotImplementedError("Methods is not supported");
  }

  virtual OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const Currency &,
      const Qty &,
      const OrderParams &) override {
    throw MethodDoesNotImplementedError("Methods is not supported");
  }

  virtual void SendCancelOrder(const OrderId &) override {
    throw MethodDoesNotImplementedError("Methods is not supported");
  }

 private:
  Settings m_settings;

  net::HTTPSClientSession m_session;
  Request m_getBalancesRequest;

  std::vector<boost::shared_ptr<Rest::Security>> m_securities;

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
