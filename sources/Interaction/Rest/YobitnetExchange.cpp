/*******************************************************************************
 *   Created: 2017/10/11 00:57:13
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
using namespace trdk::Interaction;
using namespace trdk::Interaction::Rest;

namespace pc = Poco;
namespace net = pc::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

namespace {
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

class YobitnetExchange : public TradingSystem, public MarketDataSource {
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
  explicit YobitnetExchange(const App &,
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

  virtual ~YobitnetExchange() override = default;

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

  virtual OrderId SendOrderTransaction(trdk::Security &security,
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
  const auto &result = boost::make_shared<YobitnetExchange>(
      App::GetInstance(), mode, tradingSystemIndex, marketDataSourceIndex,
      context, instanceName, configuration);
  return {result, result};
}

////////////////////////////////////////////////////////////////////////////////
