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
#include "App.hpp"
#include "FloodControl.hpp"
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
  std::string apiKey;
  std::string apiSecret;
  size_t initialNonce;
  fs::path nonceStorageFile;
  std::vector<std::string> defaultSymbols;

  explicit Settings(const IniSectionRef &conf, ModuleEventsLog &log)
      : Rest::Settings(conf, log),
        apiKey(conf.ReadKey("api_key")),
        apiSecret(conf.ReadKey("api_secret")),
        initialNonce(conf.ReadTypedKey<size_t>("initial_nonce", 1)),
        nonceStorageFile(conf.ReadFileSystemPath("nonce_storage_file_path")),
        defaultSymbols(
            conf.GetBase().ReadList("Defaults", "symbol_list", ",", false)) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) {
    log.Info(
        "API key: \"%1%\". API secret: %2%. Initial nonce: %3%. Nonce storage "
        "file: %4%",
        apiKey,                                    // 1
        apiSecret.empty() ? "not set" : "is set",  // 2
        initialNonce,                              // 3
        nonceStorageFile);                         // 4
  }

  void Validate() {
    if (initialNonce <= 0) {
      throw TradingSystem::Error("Initial nonce could not be less than 1");
    }
  }
};

class InvalidPairException : public Exception {
 public:
  explicit InvalidPairException(const char *what) noexcept : Exception(what) {}
};

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
                   const std::string &uriParams = std::string())
      : Base(uri, name, method, uriParams) {}

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
                         const std::string &uriParams = std::string())
      : Base(uri, name, net::HTTPRequest::HTTP_GET, uriParams) {}

 protected:
  virtual bool IsPriority() const { return false; }
};

class TradeRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit TradeRequest(const std::string &method,
                        size_t nonce,
                        const Settings &settings,
                        bool isPriority,
                        const std::string &uriParams = std::string())
      : Base("/tapi/",
             method,
             net::HTTPRequest::HTTP_POST,
             AppendUriParams("method=" + method + "&nonce=" +
                                 boost::lexical_cast<std::string>(nonce),
                             uriParams)),
        m_apiKey(settings.apiKey),
        m_apiSecret(settings.apiSecret),
        m_isPriority(isPriority) {
    AssertLe(0, nonce);
    AssertGe(2147483646, nonce);
  }

  virtual ~TradeRequest() override = default;

 public:
  virtual boost::tuple<pt::ptime, ptr::ptree, Milestones> Send(
      net::HTTPClientSession &session, const Context &context) override {
    auto result = Base::Send(session, context);
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
          if (*message == "invalid pair") {
            throw InvalidPairException(error.str().c_str());
          } else if (*message ==
                     "The given order has already been closed and cannot be "
                     "canceled.") {
            throw TradingSystem::OrderIsUnknown(error.str().c_str());
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
    request.set("Key", m_apiKey);
    {
      using namespace trdk::Lib::Crypto;
      const auto &digest = Hmac::CalcSha512Digest(GetUriParams(), m_apiSecret);
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
      throw Exception(error.str().c_str());
    }
    return *result;
  }

  virtual bool IsPriority() const { return m_isPriority; }

 private:
  const std::string m_apiKey;
  const std::string m_apiSecret;
  const bool m_isPriority;
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
  return source;
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
        m_marketDataSession("yobit.net"),
        m_tradingSession(m_marketDataSession.getHost()),
        m_nextNonce(0),
        m_balances(GetTsLog(), GetTsTradingLog()),
        m_pullingTask(boost::make_unique<PullingTask>(
            m_settings.pullingSetttings, GetMdsLog())) {
    m_marketDataSession.setKeepAlive(true);
    m_tradingSession.setKeepAlive(true);
  }

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
  virtual bool IsConnected() const override { return m_nextNonce > 0; }

  //! Makes connection with Market Data Source.
  virtual void Connect(const IniSectionRef &conf) override {
    // Implementation for trdk::MarketDataSource.
    if (IsConnected()) {
      return;
    }
    GetTsLog().Debug("Creating connection...");
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
        "/api/3/depth/" + boost::join(uriSymbolsPath, "-"), "Depth", "limit=1");
    m_pullingTask->ReplaceTask(
        "Prices", 1,
        [this, depthRequest]() {
          UpdateSecuritues(*depthRequest);
          return true;
        },
        m_settings.pullingSetttings.GetPricesRequestFrequency());

    m_pullingTask->AccelerateNextPulling();
  }

  virtual const Balances &GetBalances() const override { return m_balances; }

  virtual Volume CalcCommission(const Volume &vol,
                                const trdk::Security &security) const override {
    return RoundByPrecision(vol * (0.2 / 100),
                            security.GetPricePrecisionPower());
  }

  virtual bool CheckOrder(const trdk::Security &security,
                          const Currency &currency,
                          const Qty &qty,
                          const boost::optional<Price> &price,
                          const OrderSide &side,
                          bool logError) const override {
    const auto &symbol = security.GetSymbol();
    if (symbol.GetQuoteSymbol() == "BTC") {
      if (price && qty * *price < 0.0001) {
        if (logError) {
          GetTsLog().Warn(
              "Order volume restriction. Should be 0.0001 or more, but "
              "%1$.8f * %2$.8f = %3$.8f set for \"%4%\" (to %5%).",
              qty,           // 1
              *price,        // 2
              *price * qty,  // 3
              security,      // 4
              side);         // 5
        }
        return false;
      }
      if (symbol.GetSymbol() == "ETH_BTC") {
        if (qty < 0.005) {
          if (logError) {
            GetTsLog().Warn(
                "Order quantity restriction. Should be 0.005 ETH or more, but "
                "%1$.8f set for \"%2%\" (to %3%).",
                qty,       // 1
                security,  // 2
                side);     // 3
          }
          return false;
        }
      }
    }
    return TradingSystem::CheckOrder(security, currency, qty, price, side,
                                     logError);
  }

 protected:
  virtual void CreateConnection(const IniSectionRef &) override {
    {
      std::ifstream nonceStorage(m_settings.nonceStorageFile.string().c_str());
      if (!nonceStorage) {
        GetTsLog().Debug("Failed to open %1% to read.",
                         m_settings.nonceStorageFile);
      } else {
        nonceStorage.read(reinterpret_cast<char *>(&m_nextNonce),
                          sizeof(m_nextNonce));
        GetTsLog().Debug("Restored nonce-value %1% from %2%.",
                         m_nextNonce,                   // 1
                         m_settings.nonceStorageFile);  // 2
      }
      if (m_nextNonce < m_settings.initialNonce) {
        GetTsLog().Debug("Using initial nonce-value %1%.",
                         m_settings.initialNonce);
        m_nextNonce = m_settings.initialNonce;
      }
      m_nonceStorage =
          std::ofstream(m_settings.nonceStorageFile.string().c_str(),
                        std::fstream::out | std::fstream::binary);
      if (!m_nonceStorage) {
        GetTsLog().Error("Failed to open %1% to store.",
                         m_settings.nonceStorageFile);
      }
    }

    try {
      RequestAccountInfo();
      RequestProducts();
    } catch (const std::exception &ex) {
      throw ConnectError(ex.what());
    }

    Verify(m_pullingTask->AddTask(
        "Actual orders", 0,
        [this]() {
          UpdateOrders();
          return true;
        },
        m_settings.pullingSetttings.GetActualOrdersRequestFrequency()));
    Verify(m_pullingTask->AddTask(
        "Balances", 1,
        [this]() {
          RequestBalances();
          return true;
        },
        m_settings.pullingSetttings.GetBalancesRequestFrequency()));
    Verify(m_pullingTask->AddTask(
        "Opened orders", 100, [this]() { return RequestOpenedOrders(); },
        m_settings.pullingSetttings.GetAllOrdersRequestFrequency()));

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
      const auto &it = m_securities.find(product->first);
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

    boost::format requestParams("pair=%1%&type=%2%&rate=%3$.8f&amount=%4$.8f");
    requestParams % product->second.id                              // 1
        % (side == ORDER_SIDE_SELL ? "sell" : "buy")                // 2
        % RoundByPrecision(*price, product->second.precisionPower)  // 3
        % qty;                                                      // 4

    auto nonce = TakeNonce();
    TradeRequest request("Trade", nonce.first, m_settings, true,
                         requestParams.str());
    const auto &result = request.Send(m_tradingSession, GetContext());
    nonce.second.unlock();

    try {
      return boost::make_unique<OrderTransactionContext>(
          boost::get<1>(result).get<OrderId>("order_id"));
    } catch (const std::exception &ex) {
      boost::format error("Failed to read order transaction reply: \"%1%\"");
      error % ex.what();
      throw TradingSystem::Error(error.str().c_str());
    }
  }

  virtual void SendCancelOrderTransaction(const OrderId &orderId) override {
    const auto &nonce = TakeNonce();
    TradeRequest request(
        "CancelOrder", nonce.first, m_settings, true,
        "order_id=" + boost::lexical_cast<std::string>(orderId));
    request.Send(m_tradingSession, GetContext());
  }

  virtual void OnTransactionSent(const OrderId &orderId) override {
    TradingSystem::OnTransactionSent(orderId);
    m_pullingTask->AccelerateNextPulling();
  }

 private:
  std::pair<size_t, boost::mutex::scoped_lock> TakeNonce() {
    boost::mutex::scoped_lock lock(m_nonceMutex);
    m_nextNonce++;
    m_nonceStorage.seekp(0);
    m_nonceStorage.write(reinterpret_cast<const char *>(&m_nextNonce),
                         sizeof(m_nextNonce));
    m_nonceStorage.flush();
    AssertEq(sizeof(m_nextNonce), m_nonceStorage.tellp());
    return std::make_pair(m_nextNonce - 1, std::move(lock));
  }

  void UpdateOrder(const Order &order, const OrderStatus &status) {
    OnOrder(order.id, order.symbol, status, order.qty, order.qty, order.price,
            order.side, order.tid, order.time, order.time);
  }

  void RequestProducts() {
    boost::unordered_map<std::string, Product> products;
    std::vector<std::string> log;
    PublicRequest request("/api/3/info", "Info");
    try {
      const auto response =
          boost::get<1>(request.Send(m_marketDataSession, GetContext()));
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
        boost::format logStr("%1% (ID: \"%2%\", decimal places: %3%)");
        logStr % productIt.first->first   // 1
            % productIt.first->second.id  // 2
            % decimalPlaces;              // 3
        log.emplace_back(logStr.str());
      }
    } catch (const std::exception &ex) {
      boost::format error("Failed to read supported product list: \"%1%\"");
      error % ex.what();
      throw Exception(error.str().c_str());
    }
    GetTsLog().Info("Pairs: %1%.", boost::join(log, ", "));
    m_products = std::move(products);
  }

  void RequestAccountInfo() {
    std::vector<std::string> rights;
    size_t numberOfTransactions = 0;
    size_t numberOfActiveOrders = 0;

    try {
      auto nonce = TakeNonce();
      TradeRequest request("getInfo", nonce.first, m_settings, false);
      const auto response =
          boost::get<1>(request.Send(m_tradingSession, GetContext()));
      nonce.second.unlock();

      SetBalances(response);
      {
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
      }
      {
        const auto &rightsNode = response.get_child_optional("rights");
        if (rightsNode) {
          for (const auto &node : *rightsNode) {
            rights.emplace_back(node.first + ": " + node.second.data());
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
  }

  void RequestBalances() {
    auto nonce = TakeNonce();
    TradeRequest request("getInfo", nonce.first, m_settings, false);
    const auto response =
        boost::get<1>(request.Send(m_marketDataSession, GetContext()));
    nonce.second.unlock();
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

  bool RequestOpenedOrders() {
    boost::unordered_map<OrderId, Order> newOrders;
    std::vector<std::string> invalidSymbols;
    for (const auto &symbol : m_settings.defaultSymbols) {
      const auto &product = m_products.find(symbol);
      if (product == m_products.cend()) {
        continue;
      }
      try {
        RequestOpenedOrders(product->first, product->second.id, newOrders,
                            m_orders);
      } catch (const InvalidPairException &ex) {
        invalidSymbols.emplace_back(symbol);
        GetTsLog().Error(
            "Failed to request opened order list for \"%1%\": \"%2%\".",
            symbol,      // 1
            ex.what());  // 2
      } catch (const std::exception &ex) {
        boost::format error(
            "Failed to request opened order list for \"%1%\": \"%2%\"");
        error % symbol    // 1
            % ex.what();  // 2
        throw Exception(error.str().c_str());
      }
    }
    for (const auto &canceledOrder : m_orders) {
      UpdateOrder(canceledOrder.second, ORDER_STATUS_CANCELLED);
    }
    m_orders.swap(newOrders);
    for (const auto &symbol : invalidSymbols) {
      const auto it = std::find(m_settings.defaultSymbols.begin(),
                                m_settings.defaultSymbols.end(), symbol);
      Assert(it != m_settings.defaultSymbols.end());
      if (it == m_settings.defaultSymbols.end()) {
        continue;
      }
      m_settings.defaultSymbols.erase(it);
    }

    return !m_orders.empty();
  }

  void RequestOpenedOrders(
      const std::string &symbol,
      const std::string &productId,
      boost::unordered_map<OrderId, Order> &newOrders,
      boost::unordered_map<OrderId, Order> &notifiedOrders) {
    class ActiveOrdersRequest : public TradeRequest {
     public:
      typedef TradeRequest Base;

     public:
      explicit ActiveOrdersRequest(const std::string &symbol,
                                   size_t nonce,
                                   const Settings &settings)
          : Base("ActiveOrders",
                 nonce,
                 settings,
                 false,
                 "pair=" + NormilizeProductId(symbol)) {}

      virtual ~ActiveOrdersRequest() override = default;

     protected:
      virtual const ptr::ptree &ExtractContent(
          const ptr::ptree &responseTree) const {
        const auto &result = responseTree.get_child_optional("return");
        if (!result) {
          static const ptr::ptree dummy;
          return dummy;
        }
        return *result;
      }
    };

    const bool isInitial = notifiedOrders.empty();

    auto nonce = TakeNonce();
    ActiveOrdersRequest request(productId, nonce.first, m_settings);

    const auto orders =
        boost::get<1>(request.Send(m_marketDataSession, GetContext()));
    nonce.second.unlock();

    for (const auto &orderNode : orders) {
      const auto &order = orderNode.second;
      const OrderId orderId(orderNode.first);

      const Qty qty = order.get<double>("amount");

      OrderSide side;
      const auto &type = order.get<std::string>("type");
      if (type == "sell") {
        side = ORDER_SIDE_SELL;
      } else if (type == "buy") {
        side = ORDER_SIDE_BUY;
      } else {
        GetTsLog().Error("Unknown order type \"%1%\" for order %2%.", type,
                         orderId);
        continue;
      }

      const auto &time =
          pt::from_time_t(order.get<time_t>("timestamp_created"));

      const Order notifiedOrder = {std::move(orderId),
                                   std::move(symbol),
                                   std::move(qty),
                                   Price(order.get<double>("rate")),
                                   std::move(side),
                                   TIME_IN_FORCE_GTC,
                                   time};
      UpdateOrder(notifiedOrder, ORDER_STATUS_SUBMITTED);
      if (isInitial || notifiedOrders.count(notifiedOrder.id)) {
        const auto id = notifiedOrder.id;
        newOrders.emplace(id, std::move(notifiedOrder));
      }
      notifiedOrders.erase(notifiedOrder.id);
    }
  }

  void UpdateSecuritues(PublicRequest &depthRequest) {
    try {
      const auto &response =
          depthRequest.Send(m_marketDataSession, GetContext());
      const auto &time = boost::get<0>(response);
      const auto &delayMeasurement = boost::get<2>(response);
      for (const auto &updateRecord : boost::get<1>(response)) {
        const auto &symbol = updateRecord.first;
        const auto security = m_securities.find(symbol);
        if (security == m_securities.cend()) {
          GetMdsLog().Warn(
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
        security->second->SetLevel1(
            time, std::move(bestBid.first), std::move(bestBid.second),
            std::move(bestAsk.first), std::move(bestAsk.second),
            delayMeasurement);
        security->second->SetOnline(pt::not_a_date_time, true);
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
      throw MarketDataSource::Error(error.str().c_str());
    }
  }

  void UpdateOrders() {
    for (const OrderId &orderId : GetActiveOrderList()) {
      try {
        auto nonce = TakeNonce();
        TradeRequest request(
            "OrderInfo", nonce.first, m_settings, false,
            "order_id=" + boost::lexical_cast<std::string>(orderId));
        const auto response =
            boost::get<1>(request.Send(m_marketDataSession, GetContext()));
        nonce.second.unlock();
        for (const auto &node : response) {
          const auto &order = node.second;

          const auto &time =
              pt::from_time_t(order.get<time_t>("timestamp_created"));

          const Qty qty = order.get<double>("start_amount");
          const Qty remainingQty = order.get<double>("amount");

          OrderStatus status;
          switch (order.get<int>("status")) {
            case 0:  // 0 - active
              status = qty != remainingQty ? ORDER_STATUS_FILLED_PARTIALLY
                                           : ORDER_STATUS_SUBMITTED;
              break;
            case 1:  // 1 - fulfilled and closed
              status = ORDER_STATUS_FILLED;
              break;
            case 2:  // 2 - canceled
            case 3:  // 3 - canceled after partially fulfilled
              status = qty != remainingQty ? ORDER_STATUS_FILLED
                                           : ORDER_STATUS_CANCELLED;
              break;
          }

          OrderSide side;
          const auto &type = order.get<std::string>("type");
          if (type == "sell") {
            side = ORDER_SIDE_SELL;
          } else if (type == "buy") {
            side = ORDER_SIDE_BUY;
          } else {
            GetTsLog().Error("Unknown order type \"%1%\" for order %2%.", type,
                             orderId);
            continue;
          }

          OnOrder(orderId, boost::to_upper_copy(order.get<std::string>("pair")),
                  status, qty, remainingQty, Price(order.get<double>("rate")),
                  side, TIME_IN_FORCE_GTC, time, time);
        }
      } catch (const std::exception &ex) {
        boost::format error("Failed to request state for order %1%: \"%2%\"");
        error % orderId   // 1
            % ex.what();  // 2
        throw Exception(error.str().c_str());
      }
    }
  }

 private:
  Settings m_settings;

  net::HTTPSClientSession m_marketDataSession;
  net::HTTPSClientSession m_tradingSession;

  size_t m_nextNonce;
  std::ofstream m_nonceStorage;
  boost::mutex m_nonceMutex;

  boost::unordered_map<std::string, Product> m_products;

  boost::unordered_map<std::string, boost::shared_ptr<Rest::Security>>
      m_securities;
  BalancesContainer m_balances;

  std::unique_ptr<PullingTask> m_pullingTask;

  boost::unordered_map<OrderId, Order> m_orders;
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
