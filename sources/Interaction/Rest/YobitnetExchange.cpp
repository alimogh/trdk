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
#include "FloodControl.hpp"
#include "NonceStorage.hpp"
#include "PullingTask.hpp"
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
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace {

struct Settings : public Rest::Settings {
  struct Auth {
    std::string key;
    std::string secret;
    NonceStorage::Settings nonces;

    explicit Auth(const IniSectionRef &conf,
                  const char *apiKeyKey,
                  const char *apiSecretKey,
                  bool isTrading,
                  ModuleEventsLog &log)
        : key(conf.ReadKey(apiKeyKey)),
          secret(conf.ReadKey(apiSecretKey)),
          nonces(key, "Yobitnet", conf, log, isTrading) {}
  };
  Auth generalAuth;
  boost::optional<Auth> tradingAuth;

  explicit Settings(const IniSectionRef &conf, ModuleEventsLog &log)
      : Rest::Settings(conf, log),
        generalAuth(conf, "api_key", "api_secret", false, log) {
    {
      const char *apiKeyKey = "api_trading_key";
      const char *apiSecretKey = "api_trading_secret";
      if (conf.IsKeyExist(apiKeyKey) || conf.IsKeyExist(apiSecretKey)) {
        tradingAuth = Auth(conf, apiKeyKey, apiSecretKey, true, log);
      }
    }
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) {
    log.Info("General API key: \"%1%\". General API secret: %2%.",
             generalAuth.key,                                     // 1
             generalAuth.secret.empty() ? "not set" : "is set");  // 2
    if (tradingAuth) {
      log.Info("Trading API key: \"%1%\". Trading API secret: %2%.",
               tradingAuth->key,                                     // 1
               tradingAuth->secret.empty() ? "not set" : "is set");  // 2
    }
  }

  void Validate() {
    if (generalAuth.nonces.initialNonce <= 0 ||
        (tradingAuth && tradingAuth->nonces.initialNonce <= 0)) {
      throw TradingSystem::Error("Initial nonce could not be less than 1");
    }
  }
};

struct Auth {
  Settings::Auth &settings;
  NonceStorage &nonces;
};

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
template <Level1TickType priceType, Level1TickType qtyType>
boost::optional<std::pair<Level1TickValue, Level1TickValue>> ReadTopOfBook(
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
        return std::make_pair(
            Level1TickValue::Create<priceType>(std::move(price)),
            Level1TickValue::Create<qtyType>(val.second.get_value<double>()));
      }
    }
    break;
  }
  return boost::none;
}
#pragma warning(pop)

struct Product {
  std::string id;
  uintmax_t precisionPower;
};

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

 public:
  explicit Request(const std::string &uri,
                   const std::string &name,
                   const std::string &method,
                   const std::string &uriParams,
                   const Context &context,
                   ModuleEventsLog &log,
                   ModuleTradingLog *tradingLog = nullptr)
      : Base(uri, name, method, uriParams, context, log, tradingLog) {}

 protected:
  virtual FloodControl &GetFloodControl() override {
    static DisabledFloodControl result;
    return result;
  }
};

class PublicRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit PublicRequest(const std::string &uri,
                         const std::string &name,
                         const std::string &uriParams,
                         const Context &context,
                         ModuleEventsLog &log)
      : Base(uri, name, net::HTTPRequest::HTTP_GET, uriParams, context, log) {}

 protected:
  virtual bool IsPriority() const { return false; }
};

class TradeRequest : public Request {
 public:
  typedef Request Base;

  class EmptyResponseException : public Exception {
   public:
    explicit EmptyResponseException(const char *what) noexcept
        : Exception(what) {}
  };

 public:
  explicit TradeRequest(const std::string &method,
                        Auth &auth,
                        bool isPriority,
                        const std::string &uriParams,
                        const Context &context,
                        ModuleEventsLog &log,
                        ModuleTradingLog *tradingLog = nullptr)
      : Base("/tapi/",
             method,
             net::HTTPRequest::HTTP_POST,
             uriParams,
             context,
             log,
             tradingLog),
        m_auth(auth),
        m_isPriority(isPriority),
        m_nonce(m_auth.nonces.TakeNonce()) {
    AssertGe(2147483646, m_nonce->Get());
    SetUriParams(AppendUriParams(
        "method=" + method +
            "&nonce=" + boost::lexical_cast<std::string>(m_nonce->Get()),
        GetUriParams()));
  }

  virtual ~TradeRequest() override = default;

 public:
  virtual boost::tuple<pt::ptime, ptr::ptree, Milestones> Send(
      net::HTTPClientSession &session) override {
    if (!m_nonce) {
      throw LogicError("Nonce value already used");
    }
    boost::optional<NonceStorage::TakenValue> nonce(std::move(*m_nonce));
    m_nonce = boost::none;

    auto result = Base::Send(session);
    nonce->Use();

    const auto &responseTree = boost::get<1>(result);

    {
      const auto &status = responseTree.get_optional<int>("success");
      if (!status || !*status) {
        std::ostringstream error;
        error << "The server returned an error in response to the request \""
              << GetName() << "\" (" << GetRequest().getURI() << "): ";
        const auto &message = responseTree.get_optional<std::string>("error");
        if (message) {
          error << "\"" << *message << "\"";
        } else {
          error << "Unknown error";
        }
        if (message) {
          if (boost::istarts_with(*message,
                                  "The given order has already been "
                                  "closed and cannot be cancel")) {
            throw TradingSystem::OrderIsUnknown(error.str().c_str());
          } else if (boost::istarts_with(*message,
                                         "Insufficient funds in wallet of the "
                                         "second currency of the pair")) {
            throw TradingSystem::CommunicationError(error.str().c_str());
          }
        }
        throw Exception(error.str().c_str());
      }
    }

    return {boost::get<0>(result), ExtractContent(responseTree),
            boost::get<2>(result)};
  }

 protected:
  virtual void PrepareRequest(const net::HTTPClientSession &session,
                              const std::string &body,
                              net::HTTPRequest &request) const override {
    request.set("Key", m_auth.settings.key);
    {
      using namespace trdk::Lib::Crypto;
      const auto &digest =
          Hmac::CalcSha512Digest(GetUriParams(), m_auth.settings.secret);
      request.set("Sign", EncodeToHex(&digest[0], digest.size()));
    }
    Base::PrepareRequest(session, body, request);
  }

  virtual const ptr::ptree &ExtractContent(
      const ptr::ptree &responseTree) const {
    const auto &result = responseTree.get_child_optional("return");
    if (!result) {
      std::stringstream ss;
      boost::property_tree::json_parser::write_json(ss, responseTree, false);
      boost::format error(
          "The server did not return response to the request \"%1%\": "
          "\"%2%\".");
      error % GetName()  // 1
          % ss.str();    // 2
      throw EmptyResponseException(error.str().c_str());
    }
    return *result;
  }

  virtual bool IsPriority() const { return m_isPriority; }

 private:
  Auth &m_auth;
  const bool m_isPriority;
  boost::optional<NonceStorage::TakenValue> m_nonce;
};

std::string NormilizeProductId(std::string source) {
  return boost::to_lower_copy(source);
}

std::string NormilizeSymbol(std::string source) {
  if (boost::starts_with(source, "BCC_")) {
    source[2] = 'H';
  } else if (boost::ends_with(source, "_BCC")) {
    source.back() = 'H';
  }
  if (boost::starts_with(source, "USD_")) {
    source.insert(3, 1, 'T');
  } else if (boost::ends_with(source, "_USD")) {
    source.push_back('T');
  }
  return source;
}

pt::ptime ParseTimeStamp(const ptr::ptree &source,
                         const pt::time_duration &timeZoneOffset) {
  auto result = pt::from_time_t(source.get<time_t>("timestamp_created"));
  result -= timeZoneOffset;
  return result;
}

struct Order {
  OrderId id;
  std::string symbol;
  Qty qty;
  boost::optional<trdk::Price> price;
  OrderSide side;
  TimeInForce tid;
  pt::ptime time;
};

std::unique_ptr<NonceStorage> CreateTradingNoncesStorage(
    const Settings &settings, ModuleEventsLog &log) {
  if (!settings.tradingAuth) {
    return nullptr;
  }
  return boost::make_unique<NonceStorage>(settings.tradingAuth->nonces, log);
}
}

////////////////////////////////////////////////////////////////////////////////

namespace {
class YobitnetExchange : public TradingSystem, public MarketDataSource {
 public:
  explicit YobitnetExchange(const App &,
                            const TradingMode &mode,
                            Context &context,
                            const std::string &instanceName,
                            const IniSectionRef &conf)
      : TradingSystem(mode, context, instanceName),
        MarketDataSource(context, instanceName),
        m_settings(conf, GetTsLog()),
        m_serverTimeDiff(
            GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
        m_generalNonces(m_settings.generalAuth.nonces, GetTsLog()),
        m_tradingNonces(CreateTradingNoncesStorage(m_settings, GetTsLog())),
        m_generalAuth({m_settings.generalAuth, m_generalNonces}),
        m_tradingAuth(m_settings.tradingAuth
                          ? Auth{*m_settings.tradingAuth, *m_tradingNonces}
                          : Auth{m_settings.generalAuth, m_generalNonces}),
        m_marketDataSession(CreateSession("yobit.net", m_settings, false)),
        m_tradingSession(CreateSession("yobit.net", m_settings, true)),
        m_balances(GetTsLog(), GetTsTradingLog()),
        m_pullingTask(boost::make_unique<PullingTask>(
            m_settings.pullingSetttings, GetMdsLog())) {}

  virtual ~YobitnetExchange() override {
    try {
      m_pullingTask.reset();
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
  virtual bool IsConnected() const override { return !m_products.empty(); }

  //! Makes connection with Market Data Source.
  virtual void Connect(const IniSectionRef &conf) override {
    // Implementation for trdk::MarketDataSource.
    if (IsConnected()) {
      return;
    }
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
    const auto &depthRequest = boost::make_shared<PublicRequest>(
        "/api/3/depth/" + boost::join(uriSymbolsPath, "-"), "Depth", "limit=1",
        GetContext(), GetTsLog());
    m_pullingTask->ReplaceTask(
        "Prices", 1,
        [this, depthRequest]() {
          UpdatePrices(*depthRequest);
          return true;
        },
        m_settings.pullingSetttings.GetPricesRequestFrequency());

    m_pullingTask->AccelerateNextPulling();
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
    {
      const auto &result =
          TradingSystem::CheckOrder(security, currency, qty, price, side);
      if (result) {
        return result;
      }
    }
    {
      const auto &minVolume = 0.0001;
      if (price && qty * *price < minVolume) {
        return OrderCheckError{boost::none, boost::none, minVolume};
      }
    }
    if (security.GetSymbol().GetSymbol() == "ETH_BTC") {
      const auto minQty = 0.005;
      if (qty < minQty) {
        return OrderCheckError{minQty};
      }
    }
    return boost::none;
  }

 protected:
  virtual void CreateConnection(const IniSectionRef &) override {
    GetTsLog().Debug(
        "Creating connection%1%...",
        !m_settings.tradingAuth ? "" : " with general credentials");
    try {
      if (m_settings.tradingAuth) {
        Assert(&m_tradingAuth.settings != &m_generalAuth.settings);
        RequestAccountInfo(m_generalAuth);
        GetTsLog().Debug("Creating connection with trading credentials...");
      } else {
        Assert(&m_tradingAuth.settings == &m_generalAuth.settings);
      }
      if (!RequestAccountInfo(m_tradingAuth)) {
        throw ConnectError("Credentials don't have trading rights");
      }
      RequestProducts();
    } catch (const ConnectError &) {
      throw;
    } catch (const std::exception &ex) {
      throw ConnectError(ex.what());
    }

    m_pullingTask->AddTask(
        "Actual orders", 0,
        [this]() {
          UpdateOrders();
          return true;
        },
        m_settings.pullingSetttings.GetActualOrdersRequestFrequency());
    m_pullingTask->AddTask(
        "Balances", 1,
        [this]() {
          UpdateBalances();
          return true;
        },
        m_settings.pullingSetttings.GetBalancesRequestFrequency());

    m_pullingTask->AccelerateNextPulling();
  }

  virtual trdk::Security &CreateNewSecurityObject(
      const Symbol &symbol) override {
    const auto &product = m_products.find(symbol.GetSymbol());
    if (product == m_products.cend()) {
      boost::format message(
          "Symbol \"%1%\" is not in the exchange product list");
      message % symbol.GetSymbol();
      throw SymbolIsNotSupportedError(message.str().c_str());
    }

    {
      const auto &it = m_securities.find(product->second.id);
      if (it != m_securities.cend()) {
        return *it->second;
      }
    }

    const auto &result = boost::make_shared<Rest::Security>(
        GetContext(), symbol, *this,
        Rest::Security::SupportedLevel1Types()
            .set(LEVEL1_TICK_BID_PRICE)
            .set(LEVEL1_TICK_BID_QTY)
            .set(LEVEL1_TICK_ASK_PRICE)
            .set(LEVEL1_TICK_BID_QTY));
    result->SetTradingSessionState(pt::not_a_date_time, true);

    Verify(m_securities.emplace(product->second.id, result).second);

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

    const auto &product = m_products.find(security.GetSymbol().GetSymbol());
    if (product == m_products.cend()) {
      throw Exception("Symbol is not supported by exchange");
    }

    const auto &productId = product->second.id;
    const std::string actualSide = side == ORDER_SIDE_SELL ? "sell" : "buy";
    const auto actualPrice =
        RoundByPrecision(*price, product->second.precisionPower);

    boost::format requestParams("pair=%1%&type=%2%&rate=%3$.8f&amount=%4$.8f");
    requestParams % productId  // 1
        % actualSide           // 2
        % actualPrice          // 3
        % qty;                 // 4

    ptr::ptree response;
    const auto &startTime = GetContext().GetCurrentTime();
    try {
      response = boost::get<1>(TradeRequest("Trade", m_tradingAuth, true,
                                            requestParams.str(), GetContext(),
                                            GetTsLog(), &GetTsTradingLog())
                                   .Send(*m_tradingSession));
    } catch (const Request::TimeoutException &ex) {
      GetTsLog().Debug(
          "Got error \"%1%\" at the new-order request sending. Trying to "
          "retrieve actual result...",
          ex.what());
      const auto &orderId =
          FindNewOrderId(productId, startTime, actualSide, qty, actualPrice);
      if (!orderId) {
        throw;
      }
      GetTsLog().Debug("Retrieved new order ID \"%1%\".", *orderId);
      return boost::make_unique<OrderTransactionContext>(*this,
                                                         std::move(*orderId));
    }

    try {
      SetBalances(response);
      return boost::make_unique<OrderTransactionContext>(
          *this, response.get<OrderId>("order_id"));
    } catch (const std::exception &ex) {
      boost::format error("Failed to read order transaction response: \"%1%\"");
      error % ex.what();
      throw TradingSystem::Error(error.str().c_str());
    }
  }

  virtual void SendCancelOrderTransaction(const OrderId &orderId) override {
    const auto &response = boost::get<1>(
        TradeRequest("CancelOrder", m_tradingAuth, true,
                     "order_id=" + boost::lexical_cast<std::string>(orderId),
                     GetContext(), GetTsLog(), &GetTsTradingLog())
            .Send(*m_tradingSession));
    try {
      SetBalances(response);
    } catch (const std::exception &ex) {
      boost::format error(
          "Failed to read order cancel request response: \"%1%\"");
      error % ex.what();
      throw TradingSystem::Error(error.str().c_str());
    }
  }

  virtual void OnTransactionSent(const OrderId &orderId) override {
    TradingSystem::OnTransactionSent(orderId);
    m_pullingTask->AccelerateNextPulling();
  }

 private:
  void RequestProducts() {
    boost::unordered_map<std::string, Product> products;
    try {
      const auto response =
          boost::get<1>(PublicRequest("/api/3/info", "Info", std::string(),
                                      GetContext(), GetTsLog())
                            .Send(*m_marketDataSession));
      for (const auto &node : response.get_child("pairs")) {
        const auto &exchangeSymbol = boost::to_upper_copy(node.first);
        const auto &symbol = NormilizeSymbol(exchangeSymbol);
        Product product = {NormilizeProductId(exchangeSymbol)};
        const auto &info = node.second;
        const auto &decimalPlaces = info.get<uintmax_t>("decimal_places");
        product.precisionPower =
            static_cast<uintmax_t>(std::pow(10, decimalPlaces));
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
    if (products.empty()) {
      throw Exception("Exchange doesn't have products");
    }
    products.swap(m_products);
  }

  bool RequestAccountInfo(Auth &auth) {
    bool hasTradingRights = false;

    std::vector<std::string> rights;
    size_t numberOfTransactions = 0;
    size_t numberOfActiveOrders = 0;

    try {
      const auto response =
          boost::get<1>(TradeRequest("getInfo", auth, false, std::string(),
                                     GetContext(), GetTsLog())
                            .Send(*m_tradingSession));

      SetBalances(response);
      {
#if 0
        const auto &fundsInclOrdersNode =
            response.get_child_optional("funds_incl_orders");
        if (fundsInclOrdersNode) {
          for (const auto &node : *fundsInclOrdersNode) {
            GetTsLog().Info(
                "\"%1%\" balance with orders: %2$.8f.",
                boost::to_upper_copy(node.first),                  // 1
                boost::lexical_cast<Volume>(node.second.data()));  // 2
          }
        }
#endif
      }
      {
        const auto &rightsNode = response.get_child_optional("rights");
        if (rightsNode) {
          for (const auto &node : *rightsNode) {
            rights.emplace_back(node.first + ": " + node.second.data());
            if (node.first == "trade") {
              hasTradingRights = node.second.data() == "1";
            }
          }
        }
      }
      {
        const auto &numberOfTransactionsNode =
            response.get_optional<decltype(numberOfTransactions)>(
                "transaction_count");
        if (numberOfTransactionsNode) {
          numberOfTransactions = *numberOfTransactionsNode;
        }
      }
      {
        const auto &numberOfActiveOrdersNode =
            response.get_optional<decltype(numberOfTransactions)>("open_order");
        if (numberOfActiveOrdersNode) {
          numberOfActiveOrders = *numberOfActiveOrdersNode;
        }
      }
    } catch (const std::exception &ex) {
      boost::format error(
          "Failed to read general account information: \"%1%\"");
      error % ex.what();
      throw Exception(error.str().c_str());
    }

    GetTsLog().Info(
        "Rights: %1%. Number of transactions: %2%. Number of active orders: "
        "%3%.",
        rights.empty() ? "none" : boost::join(rights, ", "),  // 1
        numberOfTransactions,                                 // 2
        numberOfActiveOrders);                                // 3

    return hasTradingRights;
  }

  void UpdateBalances() {
    const auto response =
        boost::get<1>(TradeRequest("getInfo", m_generalAuth, false,
                                   std::string(), GetContext(), GetTsLog())
                          .Send(*m_marketDataSession));
    SetBalances(response);
  }

  void SetBalances(const ptr::ptree &response) {
    const auto &fundsNode = response.get_child_optional("funds");
    if (!fundsNode) {
      return;
    }
    for (const auto &node : *fundsNode) {
      m_balances.SetAvailableToTrade(
          node.first == "bcc" ? "BCH" : boost::to_upper_copy(node.first),
          boost::lexical_cast<Volume>(node.second.data()));
    }
  }

  void UpdatePrices(PublicRequest &depthRequest) {
    try {
      const auto &response = depthRequest.Send(*m_marketDataSession);
      const auto &time = boost::get<0>(response);
      const auto &delayMeasurement = boost::get<2>(response);
      for (const auto &updateRecord : boost::get<1>(response)) {
        const auto &symbol = updateRecord.first;
        const auto security = m_securities.find(symbol);
        if (security == m_securities.cend()) {
          GetMdsLog().Error(
              "Received \"depth\"-packet for unknown symbol \"%1%\".", symbol);
          continue;
        }
        const auto &data = updateRecord.second;
        const auto &bestAsk =
            ReadTopOfBook<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
                data.get_child("asks"));
        const auto &bestBid =
            ReadTopOfBook<LEVEL1_TICK_BID_PRICE, LEVEL1_TICK_BID_QTY>(
                data.get_child("bids"));
        if (bestAsk && bestBid) {
          security->second->SetLevel1(time, bestBid->first, bestBid->second,
                                      bestAsk->first, bestAsk->second,
                                      delayMeasurement);
          security->second->SetOnline(pt::not_a_date_time, true);
        } else {
          security->second->SetOnline(pt::not_a_date_time, false);
          if (bestBid) {
            security->second->SetLevel1(time, bestBid->first, bestBid->second,
                                        delayMeasurement);
          } else if (bestAsk) {
            security->second->SetLevel1(time, bestAsk->first, bestAsk->second,
                                        delayMeasurement);
          }
        }
      }
    } catch (const std::exception &ex) {
      for (auto &security : m_securities) {
        try {
          security.second->SetOnline(pt::not_a_date_time, false);
        } catch (...) {
          AssertFailNoException();
          throw;
        }
      }
      boost::format error("Failed to read \"depth\": \"%1%\"");
      error % ex.what();
      try {
        throw;
      } catch (const CommunicationError &) {
        throw CommunicationError(error.str().c_str());
      } catch (...) {
        throw Exception(error.str().c_str());
      }
    }
  }

  void UpdateOrders() {
    for (const OrderId &orderId : GetActiveOrderIdList()) {
      ptr::ptree response;
      try {
        response = boost::get<1>(
            TradeRequest(
                "OrderInfo", m_generalAuth, false,
                "order_id=" + boost::lexical_cast<std::string>(orderId),
                GetContext(), GetTsLog(), &GetTsTradingLog())
                .Send(*m_marketDataSession));
      } catch (const Exception &ex) {
        boost::format error("Failed to request state for order %1%: \"%2%\"");
        error % orderId   // 1
            % ex.what();  // 2
        try {
          throw;
        } catch (const CommunicationError &) {
          throw CommunicationError(error.str().c_str());
        } catch (...) {
          throw Exception(error.str().c_str());
        }
      }

      for (const auto &node : response) {
        const auto &order = node.second;
        pt::ptime time;
        OrderStatus status;
        Qty remainingQty = 0;
        try {
          time = ParseTimeStamp(order, m_serverTimeDiff);

          switch (order.get<int>("status")) {
            case 0: {  // 0 - active
              const auto &qty = order.get<Qty>("start_amount");
              remainingQty = order.get<Qty>("amount");
              AssertGe(qty, remainingQty);
              status = qty == remainingQty ? ORDER_STATUS_SUBMITTED
                                           : ORDER_STATUS_FILLED_PARTIALLY;
              break;
            }
            case 1:  // 1 - fulfilled and closed
            case 3:  // 3 - canceled after partially fulfilled
              status = ORDER_STATUS_FILLED;
              break;
            case 2:  // 2 - canceled
              status = ORDER_STATUS_CANCELLED;
              break;
            default:
              GetTsLog().Error(
                  "Unknown order status for order %1% (message: \"%2%\").",
                  orderId,                         // 1
                  ConvertToString(order, false));  // 2
              continue;
              break;
          }
        } catch (const std::exception &ex) {
          boost::format error("Failed to read state for order %1%: \"%2%\"");
          error % orderId   // 1
              % ex.what();  // 2
          throw Exception(error.str().c_str());
        }

        OnOrderStatusUpdate(time, orderId, status, remainingQty);
      }
    }
  }

  boost::optional<OrderId> FindNewOrderId(
      const std::string &productId,
      const boost::posix_time::ptime &startTime,
      const std::string &side,
      const Qty &qty,
      const Price &price) const {
    boost::optional<OrderId> result;

    try {
      boost::unordered_set<OrderId> orderIds;

      {
        TradeRequest request("ActiveOrders", m_tradingAuth, true,
                             "pair=" + productId, GetContext(), GetTsLog(),
                             &GetTsTradingLog());
        const auto response = boost::get<1>(request.Send(*m_tradingSession));
        for (const auto &node : response) {
          const auto &id = node.first;
          const auto &order = node.second;
          if (GetActiveOrders().count(id) ||
              order.get<std::string>("type") != side ||
              order.get<Qty>("amount") > qty ||
              order.get<Price>("rate") != price ||
              ParseTimeStamp(order, m_serverTimeDiff) + pt::minutes(2) <
                  startTime) {
            continue;
          }
          Verify(orderIds.emplace(std::move(id)).second);
        }
      }

      ForEachRemoteTrade(productId, startTime - pt::minutes(2), m_tradingAuth,
                         true,
                         [&](const std::string &, const ptr::ptree &trade) {
                           const auto &orderId = trade.get<OrderId>("order_id");
                           if (GetActiveOrders().count(orderId) ||
                               trade.get<std::string>("type") != side) {
                             return;
                           }
                           orderIds.emplace(std::move(orderId));
                         });

      for (const auto &orderId : orderIds) {
        TradeRequest request(
            "OrderInfo", m_tradingAuth, true,
            "order_id=" + boost::lexical_cast<std::string>(orderId),
            GetContext(), GetTsLog(), &GetTsTradingLog());
        const auto response = boost::get<1>(request.Send(*m_tradingSession));
        for (const auto &node : response) {
          const auto &order = node.second;
          AssertEq(orderId, node.first);
          AssertEq(0, GetActiveOrders().count(orderId));
          AssertEq(side, order.get<std::string>("type"));
          if (order.get<Qty>("start_amount") != qty ||
              order.get<Price>("rate") != price ||
              ParseTimeStamp(order, m_serverTimeDiff) + pt::minutes(2) <
                  startTime) {
            continue;
          }
          if (result) {
            GetTsLog().Warn(
                "Failed to find new order ID: Two or more orders are suitable "
                "(order ID: \"%1%\").",
                orderId);
            return boost::none;
          }
          result = std::move(orderId);
        }
      }
    } catch (const CommunicationError &ex) {
      GetTsLog().Debug("Failed to find new order ID: \"%1%\".", ex.what());
      return boost::none;
    }

    return result;
  }

  template <typename Callback>
  void ForEachRemoteTrade(const std::string &productId,
                          const pt::ptime &tradeListStartTime,
                          Auth &auth,
                          bool isPriority,
                          const Callback &callback) const {
    TradeRequest request("TradeHistory", auth, isPriority,
                         "pair=" + productId + "&since=" +
                             boost::lexical_cast<std::string>(pt::to_time_t(
                                 tradeListStartTime + m_serverTimeDiff)),
                         GetContext(), GetTsLog(), &GetTsTradingLog());
    ptr::ptree response;
    try {
      response = boost::get<1>(request.Send(*m_tradingSession));
    } catch (const TradeRequest::EmptyResponseException &) {
      return;
    }
    for (const auto &node : response) {
      callback(node.first, node.second);
    }
  }

 private:
  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  NonceStorage m_generalNonces;
  std::unique_ptr<NonceStorage> m_tradingNonces;

  mutable Auth m_generalAuth;
  mutable Auth m_tradingAuth;

  std::unique_ptr<net::HTTPClientSession> m_marketDataSession;
  std::unique_ptr<net::HTTPClientSession> m_tradingSession;

  boost::unordered_map<std::string, Product> m_products;

  boost::unordered_map<std::string, boost::shared_ptr<Rest::Security>>
      m_securities;
  BalancesContainer m_balances;

  std::unique_ptr<PullingTask> m_pullingTask;
};
}

////////////////////////////////////////////////////////////////////////////////

TradingSystemAndMarketDataSourceFactoryResult CreateYobitnet(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<YobitnetExchange>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return {result, result};
}

boost::shared_ptr<MarketDataSource> CreateYobitnetMarketDataSource(
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  return boost::make_shared<YobitnetExchange>(App::GetInstance(),
                                              TRADING_MODE_LIVE, context,
                                              instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
