/*******************************************************************************
 *   Created: 2017/10/11 00:57:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

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
template <Level1TickType priceType, Level1TickType qtyType>
std::pair<Level1TickValue, Level1TickValue> ReadTopOfBook(
    const ptr::ptree &source) {
  Price price = 0;
  Qty qty = 0;
  for (const auto &lines : source) {
    size_t i = 0;
    for (const auto &val : lines.second) {
      if (i == 0) {
        AssertEq(0, price);
        price = val.second.get_value<double>();
        ++i;
      } else {
        AssertEq(1, i);
        if (i == 1) {
          return {
              Level1TickValue::Create<priceType>(std::move(price)),
              Level1TickValue::Create<qtyType>(val.second.get_value<double>())};
        }
      }
    }
  }
  return {Level1TickValue::Create<priceType>(std::move(price)),
          Level1TickValue::Create<qtyType>(0)};
}
}

////////////////////////////////////////////////////////////////////////////////

namespace {

class Yobitnet : public TradingSystem, public MarketDataSource {
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
  explicit Yobitnet(const App &,
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
        m_session("yobit.net") {
    m_session.setKeepAlive(true);
  }

  virtual ~Yobitnet() override = default;

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
    if (m_securities.empty()) {
      return;
    }

    std::vector<std::string> uriSymbolsPath;
    uriSymbolsPath.reserve(m_securities.size());
    for (const auto &security : m_securities) {
      uriSymbolsPath.emplace_back(security.first);
    }
    const auto &uri = "/api/3/depth/" + boost::join(uriSymbolsPath, "-");
    const auto &depthRequest = boost::make_shared<Request>(
        std::move(uri), "Depth", net::HTTPRequest::HTTP_GET, m_settings.apiKey,
        m_settings.apiSecret, "limit=1");
    m_pollingTask = boost::make_unique<PollingTask>(
        [this, depthRequest]() {
          const auto &response = depthRequest->Send(m_session, GetContext());
          const auto &time = boost::get<0>(response);
          const auto &delayMeasurement = boost::get<2>(response);
          try {
            for (const auto &updateRecord : boost::get<1>(response)) {
              const auto &symbol = updateRecord.first;
              const auto security = m_securities.find(symbol);
              if (security == m_securities.cend()) {
                GetMdsLog().Warn(
                    "Received \"depth\"-packet for unknown symbol \"%1%\".",
                    symbol);
                continue;
              }
              const auto &data = updateRecord.second;
              const auto &bestAsk =
                  ReadTopOfBook<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
                      data.get_child("asks"));
              const auto &bestBid =
                  ReadTopOfBook<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
                      data.get_child("bids"));
              security->second->SetLevel1(
                  time, std::move(bestBid.first), std::move(bestBid.second),
                  std::move(bestAsk.first), std::move(bestAsk.second),
                  delayMeasurement);
            }
          } catch (const std::exception &ex) {
            boost::format error("Failed to read \"depth\": \"%1%\"");
            error % ex.what();
            throw MarketDataSource::Error(error.str().c_str());
          }
        },
        m_settings.pollingInterval, GetMdsLog());
  }

 protected:
  virtual void CreateConnection(const IniSectionRef &) override {
    m_isConnected = true;
  }

  virtual trdk::Security &CreateNewSecurityObject(
      const Symbol &symbol) override {
    {
      const auto &it = m_securities.find(symbol.GetSymbol());
      if (it != m_securities.cend()) {
        return *it->second;
      }
    }

    const auto &result = boost::make_shared<Rest::Security>(
        GetContext(), symbol, *this,
        Rest::Security::SupportedLevel1Types()
            .set(LEVEL1_TICK_BID_PRICE)
            .set(LEVEL1_TICK_ASK_PRICE));
    Verify(
        m_securities
            .emplace(std::make_pair(
                boost::to_lower_copy(result->GetSymbol().GetSymbol()), result))
            .second);
    return *result;
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

  boost::unordered_map<std::string, boost::shared_ptr<Rest::Security>>
      m_securities;

  std::unique_ptr<PollingTask> m_pollingTask;
};
}

////////////////////////////////////////////////////////////////////////////////

TradingSystemAndMarketDataSourceFactoryResult CreateYobitnet(
    const TradingMode &mode,
    size_t tradingSystemIndex,
    size_t marketDataSourceIndex,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<Yobitnet>(
      App::GetInstance(), mode, tradingSystemIndex, marketDataSourceIndex,
      context, instanceName, configuration);
  return {result, result};
}

////////////////////////////////////////////////////////////////////////////////
