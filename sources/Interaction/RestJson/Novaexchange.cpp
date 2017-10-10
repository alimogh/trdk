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
#include "RestJsonApp.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::RestJson;

namespace pc = Poco;
namespace net = pc::Net;
namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

namespace {

class Novaexchange : public TradingSystem, public MarketDataSource {
 public:
  struct Settings {
    std::string apiKey;
    std::string apiSecret;

    explicit Settings(const IniSectionRef &conf, ModuleEventsLog &log)
        : apiKey(conf.ReadKey("api_key")),
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
        m_getBalancesRequest(
            "/private/getbalances/", net::HTTPRequest::HTTP_POST, m_settings) {
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
  virtual bool IsConnected() const override { return false; }

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
    throw MethodDoesNotImplementedError("Methods is not supported");
  }

 protected:
  virtual void CreateConnection(const IniSectionRef &) override {
    GetTsLog().Debug("X %1%",
                     m_getBalancesRequest.Send<TradingSystem>(m_session));
  }

  virtual trdk::Security &CreateNewSecurityObject(const Symbol &) override {
    throw MethodDoesNotImplementedError("Methods is not supported");
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

  class Request {
   public:
    explicit Request(const std::string &uri,
                     const std::string &mehtod,
                     const Settings &settings,
                     const std::string &version = net::HTTPMessage::HTTP_1_1)
        : m_settings(settings),
          m_uri("/remote/v2" + uri),
          m_request(mehtod, m_uri, version) {
      m_request.set("User-Agent", TRDK_BUILD_IDENTITY);
      if (m_request.getMethod() == net::HTTPRequest::HTTP_POST) {
        m_request.setContentType("application/x-www-form-urlencoded");
      }
    }

   public:
    template <typename Base, typename Session>
    std::string Send(Session &session) {
      m_request.setURI(m_uri + "?nonce=" +
                       pt::to_iso_string(pt::microsec_clock::universal_time()));

      const auto &body = CreateBody();
      if (!body.empty()) {
        m_request.setContentLength(body.size());
        session.sendRequest(m_request) << body;
      } else {
        session.sendRequest(m_request);
      }

      net::HTTPResponse response;
      std::istream &responceContent = session.receiveResponse(response);
      if (response.getStatus() != 200) {
        boost::format error("%1% (HTTP-code: %2$)");
        error % response.getReason()  // 1
            % response.getStatus();   // 2
        throw Base::Error(error.str().c_str());
      }

      std::string content;
      pc::StreamCopier::copyToString(responceContent, content);
      return content;
    }

   private:
    std::string CreateBody() const {
      if (m_request.getMethod() != net::HTTPRequest::HTTP_POST) {
        return std::string();
      }
      std::ostringstream body;
      body << "apikey=" << m_settings.apiKey << "&signature=";
      {
        const auto &uri =
            std::string("https://novaexchange.com") + m_request.getURI();
        unsigned char *digest =
            HMAC(EVP_sha512(), m_settings.apiSecret.c_str(),
                 static_cast<int>(m_settings.apiSecret.size()),
                 reinterpret_cast<const unsigned char *>(uri.c_str()),
                 uri.size(), nullptr, nullptr);
        Base64Coder(false).Encode(digest, SHA512_DIGEST_LENGTH, body);
      }
      return body.str();
    }

   private:
    const Settings &m_settings;
    const std::string m_uri;
    net::HTTPRequest m_request;
  };

  Request m_getBalancesRequest;
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
