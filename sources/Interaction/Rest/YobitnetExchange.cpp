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
#include "PollingTask.hpp"
#include "Request.hpp"
#include "Security.hpp"
#include "Settings.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace TradingLib;
using namespace Interaction;
using namespace Rest;
namespace pc = Poco;
namespace net = pc::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

namespace {

const auto newOrderSeachPeriod = pt::minutes(2);

struct Settings : Rest::Settings {
  struct Auth {
    std::string key;
    std::string secret;

    explicit Auth(const ptr::ptree& conf,
                  const std::string& apiKeyKey,
                  const std::string& apiSecretKey)
        : key(conf.get<std::string>(apiKeyKey)),
          secret(conf.get<std::string>(apiSecretKey)) {}
  };

  Auth generalAuth;
  boost::optional<Auth> tradingAuth;

  explicit Settings(const ptr::ptree& conf, ModuleEventsLog& log)
      : Rest::Settings(conf, log),
        generalAuth(conf, "config.apiKey", "config.apiSecret") {
    {
      const std::string apiKeyKey = "api_trading_key";
      const std::string apiSecretKey = "api_trading_secret";
      if (conf.get_optional<std::string>(apiKeyKey) ||
          conf.get_optional<std::string>(apiSecretKey)) {
        tradingAuth = Auth(conf, apiKeyKey, apiSecretKey);
      }
    }
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog& log) {
    log.Info("General API key: \"%1%\". General API secret: %2%.",
             generalAuth.key,                                     // 1
             generalAuth.secret.empty() ? "not set" : "is set");  // 2
    if (tradingAuth) {
      log.Info("Trading API key: \"%1%\". Trading API secret: %2%.",
               tradingAuth->key,                                     // 1
               tradingAuth->secret.empty() ? "not set" : "is set");  // 2
    }
  }

  void Validate() {}
};

struct Auth {
  Settings::Auth& settings;
  NonceStorage& nonces;
};

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
template <Level1TickType priceType, Level1TickType qtyType>
boost::optional<std::pair<Level1TickValue, Level1TickValue>> ReadTopOfBook(
    const ptr::ptree& source) {
  for (const auto& lines : source) {
    double price;
    size_t i = 0;
    for (const auto& val : lines.second) {
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

  explicit Request(const std::string& uri,
                   const std::string& name,
                   const std::string& method,
                   const std::string& uriParams,
                   const Context& context,
                   ModuleEventsLog& log,
                   ModuleTradingLog* tradingLog = nullptr)
      : Base(uri, name, method, uriParams, context, log, tradingLog) {}

 protected:
  FloodControl& GetFloodControl() const override {
    static auto result = CreateDisabledFloodControl();
    return *result;
  }
};

class PublicRequest : public Request {
 public:
  typedef Request Base;

  explicit PublicRequest(const std::string& uri,
                         const std::string& name,
                         const std::string& uriParams,
                         const Context& context,
                         ModuleEventsLog& log)
      : Base(uri, name, net::HTTPRequest::HTTP_GET, uriParams, context, log) {}

 protected:
  bool IsPriority() const override { return false; }
};

class TradeRequest : public Request {
 public:
  typedef Request Base;

  class EmptyResponseException : public Exception {
   public:
    explicit EmptyResponseException(const char* what) noexcept
        : Exception(what) {}
  };

  explicit TradeRequest(const std::string& method,
                        Auth& auth,
                        bool isPriority,
                        const std::string& uriParams,
                        const Context& context,
                        ModuleEventsLog& log,
                        ModuleTradingLog* tradingLog = nullptr)
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

  ~TradeRequest() override = default;

  boost::tuple<pt::ptime, ptr::ptree, Milestones> Send(
      std::unique_ptr<net::HTTPSClientSession>& session) override {
    if (!m_nonce) {
      throw LogicError("Nonce value already used");
    }
    boost::optional<NonceStorage::TakenValue> nonce(std::move(*m_nonce));
    m_nonce = boost::none;

    auto result = Base::Send(session);
    nonce->Use();

    const auto& responseTree = boost::get<1>(result);

    {
      const auto& status = responseTree.get_optional<int>("success");
      if (!status || !*status) {
        std::ostringstream error;
        error << "The server returned an error in response to the request \""
              << GetName() << "\" (" << GetRequest().getURI() << "): ";
        const auto& message = responseTree.get_optional<std::string>("error");
        if (message) {
          error << "\"" << *message << "\"";
        } else {
          error << "Unknown error";
        }
        if (message) {
          if (boost::istarts_with(*message,
                                  "The given order has already been "
                                  "closed and cannot be cancel")) {
            throw OrderIsUnknownException(error.str().c_str());
          }
          if (boost::istarts_with(*message, "Insufficient funds")) {
            throw InsufficientFundsException(error.str().c_str());
          }
        }
        throw Exception(error.str().c_str());
      }
    }

    return {boost::get<0>(result), ExtractContent(responseTree),
            boost::get<2>(result)};
  }

 protected:
  void PrepareRequest(const net::HTTPClientSession& session,
                      const std::string& body,
                      net::HTTPRequest& request) const override {
    request.set("Key", m_auth.settings.key);
    {
      using namespace Crypto;
      const auto& digest =
          Hmac::CalcSha512Digest(GetUriParams(), m_auth.settings.secret);
      request.set("Sign", EncodeToHex(&digest[0], digest.size()));
    }
    Base::PrepareRequest(session, body, request);
  }

  virtual const ptr::ptree& ExtractContent(
      const ptr::ptree& responseTree) const {
    const auto& result = responseTree.get_child_optional("return");
    if (!result) {
      std::stringstream ss;
      ptr::json_parser::write_json(ss, responseTree, false);
      boost::format error(
          "The server did not return response to the request \"%1%\": "
          "\"%2%\".");
      error % GetName()  // 1
          % ss.str();    // 2
      throw EmptyResponseException(error.str().c_str());
    }
    return *result;
  }

  bool IsPriority() const override { return m_isPriority; }

  size_t GetNumberOfAttempts() const override { return 1; }

 private:
  Auth& m_auth;
  const bool m_isPriority;
  boost::optional<NonceStorage::TakenValue> m_nonce;
};

class YobitnetOrderTransactionContext : public OrderTransactionContext {
 public:
  explicit YobitnetOrderTransactionContext(TradingSystem& tradingSystem,
                                           OrderId&& orderId)
      : OrderTransactionContext(tradingSystem, std::move(orderId)) {}

  bool RegisterTrade(const std::string& id) {
    return m_trades.emplace(id).second;
  }

 private:
  boost::unordered_set<std::string> m_trades;
};

std::string NormilizeProductId(const std::string source) {
  return boost::to_lower_copy(source);
}

std::string NormilizeSymbol(std::string source) {
  if (boost::starts_with(source, "BCC_")) {
    source[2] = 'H';
  } else if (boost::ends_with(source, "_BCC")) {
    source.back() = 'H';
  }
  return source;
}

std::string NormilizeCurrency(std::string source) {
  if (source == "bcc") {
    source = "BCH";
  } else {
    boost::to_upper(source);
  }
  return source;
}

class YobitBalancesContainer : public BalancesContainer {
 public:
  explicit YobitBalancesContainer(const TradingSystem& tradingSystem,
                                  ModuleEventsLog& log,
                                  ModuleTradingLog& tradingLog)
      : BalancesContainer(tradingSystem, log, tradingLog) {}

  ~YobitBalancesContainer() override = default;

  void ReduceAvailableToTradeByOrder(const trdk::Security&,
                                     const Qty&,
                                     const Price&,
                                     const OrderSide&,
                                     const TradingSystem&) override {
    // Yobit.net sends new rests at order transaction reply.
  }
};
}  // namespace

////////////////////////////////////////////////////////////////////////////////

namespace {
class YobitnetExchange : public TradingSystem, public MarketDataSource {
 public:
  explicit YobitnetExchange(const App&,
                            const TradingMode& mode,
                            Context& context,
                            const std::string& instanceName,
                            const ptr::ptree& conf)
      : TradingSystem(mode, context, instanceName),
        MarketDataSource(context, instanceName),
        m_settings(conf, GetTsLog()),
        m_serverTimeDiff(
            GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
        m_generalNonces(
            boost::make_unique<NonceStorage::Int32TimedGenerator>()),
        m_tradingNonces(
            m_settings.tradingAuth
                ? boost::make_unique<NonceStorage>(
                      boost::make_unique<NonceStorage::Int32TimedGenerator>())
                : nullptr),
        m_generalAuth({m_settings.generalAuth, m_generalNonces}),
        m_tradingAuth(m_settings.tradingAuth
                          ? Auth{*m_settings.tradingAuth, *m_tradingNonces}
                          : Auth{m_settings.generalAuth, m_generalNonces}),
        m_marketDataSession(CreateSession("yobit.net", m_settings, false)),
        m_tradingSession(CreateSession("yobit.net", m_settings, true)),
        m_balances(*this, GetTsLog(), GetTsTradingLog()),
        m_pollingTask(boost::make_unique<PollingTask>(
            m_settings.pollingSetttings, GetMdsLog())) {}

  ~YobitnetExchange() override {
    try {
      m_pollingTask.reset();
      // Each object, that implements CreateNewSecurityObject should wait for
      // log flushing before destroying objects:
      MarketDataSource::GetTradingLog().WaitForFlush();
      TradingSystem::GetTradingLog().WaitForFlush();
    } catch (...) {
      AssertFailNoException();
      terminate();
    }
  }

  using trdk::TradingSystem::GetContext;

  TradingSystem::Log& GetTsLog() const noexcept {
    return TradingSystem::GetLog();
  }
  TradingSystem::TradingLog& GetTsTradingLog() const noexcept {
    return TradingSystem::GetTradingLog();
  }

  MarketDataSource::Log& GetMdsLog() const noexcept {
    return MarketDataSource::GetLog();
  }

  bool IsConnected() const override { return !m_products.empty(); }

  //! Makes connection with Market Data Source.
  void Connect() override {
    // Implementation for trdk::MarketDataSource.
    if (IsConnected()) {
      return;
    }
    CreateConnection();
  }

  void SubscribeToSecurities() override {
    if (m_securities.empty()) {
      return;
    }

    std::vector<std::string> uriSymbolsPath;
    uriSymbolsPath.reserve(m_securities.size());
    for (const auto& security : m_securities) {
      uriSymbolsPath.emplace_back(security.first);
    }
    const auto& depthRequest = boost::make_shared<PublicRequest>(
        "/api/3/depth/" + boost::join(uriSymbolsPath, "-"), "Depth", "limit=1",
        GetContext(), GetTsLog());
    m_pollingTask->ReplaceTask(
        "Prices", 1,
        [this, depthRequest]() {
          UpdatePrices(*depthRequest);
          return true;
        },
        m_settings.pollingSetttings.GetPricesRequestFrequency(), false);

    m_pollingTask->AccelerateNextPolling();
  }

  Balances& GetBalancesStorage() override { return m_balances; }

  Volume CalcCommission(const Qty& qty,
                        const Price& price,
                        const OrderSide&,
                        const trdk::Security&) const override {
    return (qty * price) * (0.2 / 100);
  }

  boost::optional<OrderCheckError> CheckOrder(
      const trdk::Security& security,
      const Currency& currency,
      const Qty& qty,
      const boost::optional<Price>& price,
      const OrderSide& side) const override {
    {
      const auto& result =
          TradingSystem::CheckOrder(security, currency, qty, price, side);
      if (result) {
        return result;
      }
    }
    {
      const auto& minVolume = 0.0001;
      if (price && qty * *price < minVolume) {
        return OrderCheckError{boost::none, boost::none, minVolume};
      }
    }
    {
      const auto& symbol = security.GetSymbol().GetSymbol();
      const auto& minQty =
          symbol == "ETH_BTC" ? 0.005 : symbol == "LTC_ETH" ? 0.0001 : 0;
      if (qty < minQty) {
        return OrderCheckError{minQty};
      }
    }
    return boost::none;
  }

  bool CheckSymbol(const std::string& symbol) const override {
    return TradingSystem::CheckSymbol(symbol) && m_products.count(symbol) > 0;
  }

 protected:
  void CreateConnection() override {
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
    } catch (const ConnectError&) {
      throw;
    } catch (const std::exception& ex) {
      throw ConnectError(ex.what());
    }

    m_pollingTask->AddTask(
        "Actual orders", 0,
        [this]() {
          UpdateOrders();
          return true;
        },
        m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
    m_pollingTask->AddTask(
        "Balances", 1,
        [this]() {
          UpdateBalances();
          return true;
        },
        m_settings.pollingSetttings.GetBalancesRequestFrequency(), false);

    m_pollingTask->AccelerateNextPolling();
  }

  trdk::Security& CreateNewSecurityObject(const Symbol& symbol) override {
    const auto& product = m_products.find(symbol.GetSymbol());
    if (product == m_products.cend()) {
      boost::format message(
          "Symbol \"%1%\" is not in the exchange product list");
      message % symbol.GetSymbol();
      throw SymbolIsNotSupportedException(message.str().c_str());
    }

    {
      const auto& it = m_securities.find(product->second.id);
      if (it != m_securities.cend()) {
        return *it->second;
      }
    }

    const auto& result = boost::make_shared<Rest::Security>(
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

  std::unique_ptr<OrderTransactionContext> SendOrderTransaction(
      trdk::Security& security,
      const Currency& currency,
      const Qty& qty,
      const boost::optional<Price>& price,
      const OrderParams& params,
      const OrderSide& side,
      const TimeInForce& tif) override {
    static_assert(numberOfTimeInForces == 5, "List changed.");
    switch (tif) {
      case TIME_IN_FORCE_IOC:
        return SendOrderTransactionAndEmulateIoc(security, currency, qty, price,
                                                 params, side);
      case TIME_IN_FORCE_GTC:
        break;
      default:
        throw Exception("Order time-in-force type is not supported");
    }
    if (currency != security.GetSymbol().GetCurrency()) {
      throw Exception("Trading system supports only security quote currency");
    }
    if (!price) {
      throw Exception("Market order is not supported");
    }

    const auto& product = m_products.find(security.GetSymbol().GetSymbol());
    if (product == m_products.cend()) {
      throw Exception("Symbol is not supported by exchange");
    }

    const auto& productId = product->second.id;
    const std::string actualSide = side == ORDER_SIDE_SELL ? "sell" : "buy";
    const auto actualPrice =
        RoundByPrecisionPower(*price, product->second.precisionPower);

    boost::format requestParams("pair=%1%&type=%2%&rate=%3%&amount=%4%");
    requestParams % productId  // 1
        % actualSide           // 2
        % actualPrice          // 3
        % qty;                 // 4

    ptr::ptree response;
    const auto& startTime = GetContext().GetCurrentTime();
    TradeRequest request("Trade", m_tradingAuth, true, requestParams.str(),
                         GetContext(), GetTsLog(), &GetTsTradingLog());
    try {
      response = boost::get<1>(request.Send(m_tradingSession));
    } catch (
        const Request::CommunicationErrorWithUndeterminedRemoteResult& ex) {
      GetTsLog().Info(
          "Got error \"%1%\" at the new-order request sending. Trying to "
          "retrieve actual result...",
          ex.what());
      auto orderId =
          FindNewOrderId(productId, startTime, actualSide, qty, actualPrice);
      if (!orderId) {
        throw;
      }
      GetTsLog().Debug("Retrieved new order ID \"%1%\".", *orderId);
      return boost::make_unique<YobitnetOrderTransactionContext>(
          *this, std::move(*orderId));
    }

    try {
      SetBalances(response);
      auto result = boost::make_unique<YobitnetOrderTransactionContext>(
          *this, response.get<OrderId>("order_id"));
      RegisterLastOrder(startTime, result->GetOrderId());
      return result;
    } catch (const ptr::ptree_error& ex) {
      boost::format error(
          "Wrong server response to the request \"%1%\" (%2%): \"%3%\"");
      error % request.GetName()            // 1
          % request.GetRequest().getURI()  // 2
          % ex.what();                     // 3
      throw Exception(error.str().c_str());
    }
  }

  void SendCancelOrderTransaction(
      const OrderTransactionContext& transaction) override {
    TradeRequest request("CancelOrder", m_tradingAuth, true,
                         "order_id=" + boost::lexical_cast<std::string>(
                                           transaction.GetOrderId()),
                         GetContext(), GetTsLog(), &GetTsTradingLog());
    const auto response = boost::get<1>(request.Send(m_tradingSession));
    try {
      SetBalances(response);
    } catch (const ptr::ptree_error& ex) {
      boost::format error(
          "Wrong server response to the request \"%1%\" (%2%): \"%3%\"");
      error % request.GetName()            // 1
          % request.GetRequest().getURI()  // 2
          % ex.what();                     // 3
      throw Exception(error.str().c_str());
    }
  }

 private:
  void RequestProducts() {
    boost::unordered_map<std::string, Product> products;
    try {
      const auto response =
          boost::get<1>(PublicRequest("/api/3/info", "Info", std::string(),
                                      GetContext(), GetTsLog())
                            .Send(m_marketDataSession));
      for (const auto& node : response.get_child("pairs")) {
        const auto& exchangeSymbol = boost::to_upper_copy(node.first);
        const auto& symbol = NormilizeSymbol(exchangeSymbol);
        Product product = {NormilizeProductId(exchangeSymbol)};
        const auto& info = node.second;
        const auto& decimalPlaces = info.get<uintmax_t>("decimal_places");
        product.precisionPower =
            static_cast<uintmax_t>(std::pow(10, decimalPlaces));
        const auto& productIt =
            products.emplace(std::move(symbol), std::move(product));
        if (!productIt.second) {
          GetTsLog().Error("Product duplicate: \"%1%\"",
                           productIt.first->first);
          Assert(productIt.second);
        }
      }
    } catch (const std::exception& ex) {
      boost::format error("Failed to read supported product list: \"%1%\"");
      error % ex.what();
      throw Exception(error.str().c_str());
    }
    if (products.empty()) {
      throw Exception("Exchange doesn't have products");
    }
    products.swap(m_products);
  }

  bool RequestAccountInfo(Auth& auth) {
    bool hasTradingRights = false;

    std::vector<std::string> rights;
    size_t numberOfTransactions = 0;
    size_t numberOfActiveOrders = 0;

    try {
      const auto response =
          boost::get<1>(TradeRequest("getInfo", auth, false, std::string(),
                                     GetContext(), GetTsLog())
                            .Send(m_tradingSession));

      SetBalances(response);
      {
        const auto& rightsNode = response.get_child_optional("rights");
        if (rightsNode) {
          for (const auto& node : *rightsNode) {
            rights.emplace_back(node.first + ": " + node.second.data());
            if (node.first == "trade") {
              hasTradingRights = node.second.data() == "1";
            }
          }
        }
      }
      {
        const auto& numberOfTransactionsNode =
            response.get_optional<decltype(numberOfTransactions)>(
                "transaction_count");
        if (numberOfTransactionsNode) {
          numberOfTransactions = *numberOfTransactionsNode;
        }
      }
      {
        const auto& numberOfActiveOrdersNode =
            response.get_optional<decltype(numberOfTransactions)>("open_order");
        if (numberOfActiveOrdersNode) {
          numberOfActiveOrders = *numberOfActiveOrdersNode;
        }
      }
    } catch (const std::exception& ex) {
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
                          .Send(m_marketDataSession));
    SetBalances(response);
  }

  void SetBalances(const ptr::ptree& response) {
    boost::unordered_map<std::string, Volume> balances;
    {
      const auto& fundsInclOrdersNode =
          response.get_child_optional("funds_incl_orders");
      if (fundsInclOrdersNode) {
        for (const auto& node : *fundsInclOrdersNode) {
          balances[NormilizeCurrency(node.first)] =
              boost::lexical_cast<Volume>(node.second.data());
        }
      }
    }
    {
      const auto& fundsNode = response.get_child_optional("funds");
      if (fundsNode) {
        for (const auto& node : *fundsNode) {
          const auto& balance = balances.find(NormilizeCurrency(node.first));
          Assert(balance != balances.cend());
          if (balance == balances.cend()) {
            continue;
          }
          auto available = boost::lexical_cast<Volume>(node.second.data());
          AssertLe(available, balance->second);
          auto locked = balance->second - available;
          m_balances.Set(balance->first, std::move(available),
                         std::move(locked));
        }
      }
    }
  }

  void UpdatePrices(PublicRequest& depthRequest) {
    try {
      const auto& response = depthRequest.Send(m_marketDataSession);
      const auto& time = boost::get<0>(response);
      const auto& delayMeasurement = boost::get<2>(response);
      for (const auto& updateRecord : boost::get<1>(response)) {
        const auto& symbol = updateRecord.first;
        const auto security = m_securities.find(symbol);
        if (security == m_securities.cend()) {
          GetMdsLog().Error(
              "Received \"depth\"-packet for unknown symbol \"%1%\".", symbol);
          continue;
        }
        const auto& data = updateRecord.second;
        const auto& bestAsk =
            ReadTopOfBook<LEVEL1_TICK_ASK_PRICE, LEVEL1_TICK_ASK_QTY>(
                data.get_child("asks"));
        const auto& bestBid =
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
    } catch (const std::exception& ex) {
      for (auto& security : m_securities) {
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
      } catch (const CommunicationError&) {
        throw CommunicationError(error.str().c_str());
      } catch (...) {
        throw Exception(error.str().c_str());
      }
    }
  }

  void UpdateOrders() {
    std::vector<boost::tuple<ptr::ptree, pt::ptime, int>> orders;
    boost::unordered_map<std::string, pt::ptime> tradesRequest;
    {
      const auto& activeOrders = GetActiveOrderContextList();
      orders.reserve(activeOrders.size());
      for (const auto& context : activeOrders) {
        const auto& orderId = context->GetOrderId();
        try {
          orders.emplace_back(boost::make_tuple(
              boost::get<1>(
                  TradeRequest(
                      "OrderInfo", m_generalAuth, false,
                      "order_id=" + boost ::lexical_cast<std ::string>(orderId),
                      GetContext(), GetTsLog(), &GetTsTradingLog())
                      .Send(m_marketDataSession)),
              pt::not_a_date_time, std::numeric_limits<int>::max()));
        } catch (const Exception& ex) {
          boost::format error("Failed to request state for order %1%: \"%2%\"");
          error % orderId   // 1
              % ex.what();  // 2
          try {
            throw;
          } catch (const CommunicationError&) {
            throw CommunicationError(error.str().c_str());
          } catch (...) {
            throw Exception(error.str().c_str());
          }
        }
        auto& orderTime = boost::get<1>(orders.back());
        auto& statusCode = boost::get<2>(orders.back());
        for (auto& node : boost::get<0>(orders.back())) {
          const auto& order = node.second;
          try {
            AssertEq(orderTime, pt::not_a_date_time);
            orderTime = ParseTimeStamp(order, "timestamp_created");
            AssertEq(std::numeric_limits<int>::max(), statusCode);
            statusCode = order.get<int>("status");
            if (statusCode == 0) {
              // 0 - active
              OnOrderOpened(orderTime, orderId);
            }
            const auto& product = order.get<std::string>("pair");
            const auto& requestTime = orderTime - pt::seconds(1);
            const auto& it = tradesRequest.emplace(product, requestTime);
            if (!it.second && it.first->second > requestTime) {
              it.first->second = requestTime;
            }
          } catch (const std::exception& ex) {
            boost::format error("Failed to read state for order %1%: \"%2%\"");
            error % orderId   // 1
                % ex.what();  // 2
            throw Exception(error.str().c_str());
          }
        }
      }
    }

    boost::unordered_map<OrderId, std::map<size_t, std::pair<pt::ptime, Trade>>>
        trades;
    for (const auto& request : tradesRequest) {
      ForEachRemoteTrade(
          request.first, request.second, m_marketDataSession, m_generalAuth,
          false, [&](const std::string& id, const ptr::ptree& data) {
            trades[data.get<size_t>("order_id")].emplace(
                boost::lexical_cast<size_t>(id),
                std::make_pair(ParseTimeStamp(data, "timestamp"),
                               Trade{data.get<Price>("rate"),
                                     data.get<Qty>("amount"), id}));
          });
    }

    for (const auto& node : orders) {
      for (const auto& order : boost::get<0>(node)) {
        UpdateOrder(boost::get<1>(node), order.first, boost::get<2>(node),
                    order.second, trades, tradesRequest);
      }
    }
  }

  void UpdateOrder(
      pt::ptime time,
      const OrderId& id,
      const int statusCode,
      const ptr::ptree& source,
      boost::unordered_map<OrderId,
                           std::map<size_t, std::pair<pt::ptime, Trade>>>&
          trades,
      const boost::unordered_map<std::string, pt::ptime>& tradesRequest) {
    OrderStatus status;
    try {
      switch (statusCode) {
        case 0: {
          // 0 - active
          const auto& qty = source.get<Qty>("start_amount");
          const auto& remainingQty = source.get<Qty>("amount");
          AssertGe(qty, remainingQty);
          status = qty == remainingQty ? ORDER_STATUS_OPENED
                                       : ORDER_STATUS_FILLED_PARTIALLY;
          break;
        }
        case 1:  // 1 - fulfilled and closed
          status = ORDER_STATUS_FILLED_FULLY;
          break;
        case 2:  // 2 - canceled
        case 3:  // 3 - canceled after partially fulfilled
          status = ORDER_STATUS_CANCELED;
          break;
        default:
          status = ORDER_STATUS_ERROR;
          break;
      }
    } catch (const std::exception& ex) {
      boost::format error("Failed to read state for order %1%: \"%2%\"");
      error % id        // 1
          % ex.what();  // 2
      throw Exception(error.str().c_str());
    }

    const auto& tradesIt = trades.find(id);
    if (tradesIt == trades.cend()) {
      if (status == ORDER_STATUS_FILLED_FULLY) {
        const auto& requestIt =
            tradesRequest.find(source.get<std::string>("pair"));
        Assert(requestIt != tradesRequest.cend());
        GetTsLog().Error(
            "Received order status \"%1%\" for order \"%2%\", but didn't "
            "receive trade. Trade request start time: %3%.",
            status,                                                // 1
            id,                                                    // 2
            requestIt != tradesRequest.cend() ? requestIt->second  // 3
                                              : pt::not_a_date_time);
      }
    } else {
      if (status == ORDER_STATUS_OPENED) {
        status = ORDER_STATUS_FILLED_PARTIALLY;
      }
      for (auto& node : tradesIt->second) {
        const auto& tradeTime = node.second.first;
        AssertLe(time, tradeTime);
        if (time < tradeTime) {
          time = tradeTime;
        }
        auto& trade = node.second.second;
        const auto tradeId = trade.id;
        OnTrade(
            tradeTime, id, std::move(trade),
            [&tradeId](OrderTransactionContext& orderContext) {
              return boost::polymorphic_cast<YobitnetOrderTransactionContext*>(
                         &orderContext)
                  ->RegisterTrade(tradeId);
            });
      }
    }

    static_assert(numberOfOrderStatuses == 7, "List changed.");
    switch (status) {
      default:
        AssertEq(ORDER_STATUS_FILLED_PARTIALLY, status);
      case ORDER_STATUS_FILLED_PARTIALLY:
        break;
      case ORDER_STATUS_FILLED_FULLY:
        OnOrderFilled(time, id, boost::none);
        break;
      case ORDER_STATUS_OPENED:
        // Notified immediately after receiving.
        break;
      case ORDER_STATUS_CANCELED:
        try {
          OnOrderCanceled(time, id, boost::none, boost::none);
        } catch (OrderIsUnknownException&) {
          // This is workaround for Yobit bug. See also
          // https://yobit.net/en/support/e7f3cdb97d7dd1fa200f8b0dc8593f573fa07f9bdf309d43711c72381d39121d
          // and https://trello.com/c/luVuGQH2 for details.
        }
        break;
      case ORDER_STATUS_REJECTED:
        OnOrderRejected(time, id, boost::none, boost::none);
        break;
      case ORDER_STATUS_ERROR:
        OnOrderError(time, id, boost::none, boost::none,
                     "Unknown order status");
        break;
    }
  }

  boost::optional<OrderId> FindNewOrderId(
      const std::string& productId,
      const boost::posix_time::ptime& startTime,
      const std::string& side,
      const Qty& qty,
      const Price& price) const {
    boost::optional<OrderId> result;

    try {
      boost::unordered_set<OrderId> orderIds;

      ForEachRemoteActiveOrder(
          productId, m_tradingSession, m_tradingAuth, true,
          [&](const std::string& orderId, const ptr::ptree& order) {
            if (IsIdRegisterInLastOrders(orderId) ||
                GetActiveOrders().count(orderId) ||
                order.get<std::string>("type") != side ||
                order.get<Qty>("amount") > qty ||
                order.get<Price>("rate") != price ||
                ParseTimeStamp(order, "timestamp_created") +
                        newOrderSeachPeriod <
                    startTime) {
              return;
            }
            Verify(orderIds.emplace(std::move(orderId)).second);
          });
      ForEachRemoteTrade(productId, startTime - newOrderSeachPeriod,
                         m_tradingSession, m_tradingAuth, true,
                         [&](const std::string&, const ptr::ptree& trade) {
                           const auto& orderId = trade.get<OrderId>("order_id");
                           if (GetActiveOrders().count(orderId) ||
                               trade.get<std::string>("type") != side) {
                             return;
                           }
                           orderIds.emplace(std::move(orderId));
                         });

      for (const auto& orderId : orderIds) {
        TradeRequest request(
            "OrderInfo", m_tradingAuth, true,
            "order_id=" + boost::lexical_cast<std::string>(orderId),
            GetContext(), GetTsLog(), &GetTsTradingLog());
        const auto response = boost::get<1>(request.Send(m_tradingSession));
        for (const auto& node : response) {
          const auto& order = node.second;
          AssertEq(orderId, node.first);
          AssertEq(0, GetActiveOrders().count(orderId));
          AssertEq(side, order.get<std::string>("type"));
          if (order.get<Qty>("start_amount") != qty ||
              order.get<Price>("rate") != price ||
              ParseTimeStamp(order, "timestamp_created") + newOrderSeachPeriod <
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
    } catch (const CommunicationError& ex) {
      GetTsLog().Debug("Failed to find new order ID: \"%1%\".", ex.what());
      return boost::none;
    }

    return result;
  }

  template <typename Callback>
  void ForEachRemoteActiveOrder(
      const std::string& productId,
      std::unique_ptr<net::HTTPSClientSession>& session,
      Auth& auth,
      bool isPriority,
      const Callback& callback) const {
    TradeRequest request("ActiveOrders", auth, isPriority, "pair=" + productId,
                         GetContext(), GetTsLog(), &GetTsTradingLog());
    ptr::ptree response;
    try {
      response = boost::get<1>(request.Send(session));
    } catch (const TradeRequest::EmptyResponseException&) {
      return;
    }
    for (const auto& node : response) {
      callback(node.first, node.second);
    }
  }

  template <typename Callback>
  void ForEachRemoteTrade(const std::string& productId,
                          const pt::ptime& tradeListStartTime,
                          std::unique_ptr<net::HTTPSClientSession>& session,
                          Auth& auth,
                          bool isPriority,
                          const Callback& callback) const {
    TradeRequest request("TradeHistory", auth, isPriority,
                         "pair=" + productId + "&since=" +
                             boost::lexical_cast<std::string>(pt::to_time_t(
                                 tradeListStartTime + m_serverTimeDiff)),
                         GetContext(), GetTsLog(), &GetTsTradingLog());
    ptr::ptree response;
    try {
      response = boost::get<1>(request.Send(session));
    } catch (const TradeRequest::EmptyResponseException&) {
      return;
    }
    for (const auto& node : response) {
      callback(node.first, node.second);
    }
  }

  pt::ptime ParseTimeStamp(const ptr::ptree& source,
                           const std::string& key) const {
    auto result = pt::from_time_t(source.get<time_t>(key));
    result -= m_serverTimeDiff;
    return result;
  }

  void RegisterLastOrder(const pt::ptime& startTime, const OrderId& id) {
    const auto& start = startTime - newOrderSeachPeriod;
    while (!m_lastOrders.empty() && m_lastOrders.front().first < start) {
      m_lastOrders.pop_front();
    }
    m_lastOrders.emplace_back(startTime, id.GetValue());
  }

  bool IsIdRegisterInLastOrders(const std::string& id) const {
    for (const auto& order : m_lastOrders) {
      if (order.second == id) {
        return true;
      }
    }
    return false;
  }

  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  NonceStorage m_generalNonces;
  std::unique_ptr<NonceStorage> m_tradingNonces;

  mutable Auth m_generalAuth;
  mutable Auth m_tradingAuth;

  mutable std::unique_ptr<net::HTTPSClientSession> m_marketDataSession;
  mutable std::unique_ptr<net::HTTPSClientSession> m_tradingSession;

  boost::unordered_map<std::string, Product> m_products;

  boost::unordered_map<std::string, boost::shared_ptr<Rest::Security>>
      m_securities;
  YobitBalancesContainer m_balances;

  std::deque<std::pair<pt::ptime, std::string>> m_lastOrders;

  std::unique_ptr<PollingTask> m_pollingTask;
};
}  // namespace

////////////////////////////////////////////////////////////////////////////////

TradingSystemAndMarketDataSourceFactoryResult CreateYobitnet(
    const TradingMode& mode,
    Context& context,
    const std::string& instanceName,
    const ptr::ptree& configuration) {
  const auto& result = boost::make_shared<YobitnetExchange>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return {result, result};
}

boost::shared_ptr<MarketDataSource> CreateYobitnetMarketDataSource(
    Context& context,
    const std::string& instanceName,
    const ptr::ptree& configuration) {
  return boost::make_shared<YobitnetExchange>(App::GetInstance(),
                                              TRADING_MODE_LIVE, context,
                                              instanceName, configuration);
}

////////////////////////////////////////////////////////////////////////////////
