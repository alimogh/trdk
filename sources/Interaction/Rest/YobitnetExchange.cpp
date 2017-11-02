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
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////

namespace {

struct Settings {
  std::string apiKey;
  std::string apiSecret;
  pt::time_duration pollingInterval;
  size_t initialNonce;
  fs::path nonceStorageFile;
  std::vector<std::string> defaultSymbols;

  explicit Settings(const IniSectionRef &conf, ModuleEventsLog &log)
      : apiKey(conf.ReadKey("api_key")),
        apiSecret(conf.ReadKey("api_secret")),
        pollingInterval(pt::milliseconds(
            conf.ReadTypedKey<long>("polling_interval_milliseconds"))),
        initialNonce(conf.ReadTypedKey<size_t>("initial_nonce", 1)),
        nonceStorageFile(conf.ReadFileSystemPath("nonce_storage_file_path")),
        defaultSymbols(
            conf.GetBase().ReadList("Defaults", "symbol_list", ",", false)) {
    Log(log);
    Validate();
  }

  void Log(ModuleEventsLog &log) {
    log.Info(
        "API key: \"%1%\". API secret: %2%. Polling Interval: %3%. Initial "
        "nonce: %4%. Nonce storage file: %5%",
        apiKey,                                    // 1
        apiSecret.empty() ? "not set" : "is set",  // 2
        pollingInterval,                           // 3
        initialNonce,                              // 4
        nonceStorageFile);                         // 5
  }

  void Validate() {
    if (initialNonce <= 0) {
      throw TradingSystem::Error("Initial nonce could not be less than 1");
    }
  }
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

class Request : public Rest::Request {
 public:
  typedef Rest::Request Base;

 public:
  explicit Request(const std::string &uri,
                   const std::string &name,
                   const std::string &method,
                   const std::string &uriParams = std::string())
      : Base(uri, name, method, uriParams) {}
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

class TradeRequest : public Request {
 public:
  typedef Request Base;

 public:
  explicit TradeRequest(const std::string &method,
                        size_t nonce,
                        const Settings &settings,
                        const std::string &uriParams = std::string())
      : Base("/tapi/",
             method,
             net::HTTPRequest::HTTP_POST,
             AppendUriParams("method=" + method + "&nonce=" +
                                 boost::lexical_cast<std::string>(nonce),
                             uriParams)),
        m_apiKey(settings.apiKey),
        m_apiSecret(settings.apiSecret) {
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
        throw Exception(error.str().c_str());
      }
    }

    return boost::make_tuple(std::move(boost::get<0>(result)),
                             std::move(ExtractContent(responseTree)),
                             std::move(boost::get<2>(result)));
  }

 protected:
  virtual void PreprareRequest(const net::HTTPClientSession &,
                               net::HTTPRequest &request) const override {
    request.set("Key", m_apiKey);
    {
      using namespace trdk::Lib::Crypto;
      const auto &digest = CalcHmacSha512Digest(GetUriParams(), m_apiSecret);
      request.set("Sign", EncodeToHex(&digest[0], digest.size()));
    }
  }

  virtual const ptr::ptree &ExtractContent(
      const ptr::ptree &responseTree) const {
    const auto &result = responseTree.get_child_optional("return");
    if (!result) {
      boost::format error(
          "The server did not return response to the request \"%1%\"");
      error % GetName();
      throw Exception(error.str().c_str());
    }
    return *result;
  }

 private:
  const std::string m_apiKey;
  const std::string m_apiSecret;
};

std::string NormilizeSymbol(const std::string &source) {
  return boost::to_lower_copy(source);
}

struct Order {
  OrderId id;
  std::string tradingSystemOrderId;
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
                            size_t tradingSystemIndex,
                            size_t marketDataSourceIndex,
                            Context &context,
                            const std::string &instanceName,
                            const IniSectionRef &conf)
      : TradingSystem(mode, tradingSystemIndex, context, instanceName),
        MarketDataSource(marketDataSourceIndex, context, instanceName),
        m_settings(conf, GetTsLog()),
        m_marketDataSession("yobit.net"),
        m_tradingSession(m_marketDataSession.getHost()),
        m_nextNonce(0) {
    m_marketDataSession.setKeepAlive(true);
    m_tradingSession.setKeepAlive(true);
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
  virtual bool IsConnected() const override { return m_nextNonce > 0; }

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
    m_pollingTask = boost::make_unique<PollingTask>(
        [this, depthRequest]() {
          UpdateSecuritues(*depthRequest);
          UpdateOrders();
        },
        m_settings.pollingInterval, GetMdsLog());
  }

 protected:
  virtual void CreateConnection(const IniSectionRef &) override {
    auto nextNonce = m_nextNonce;
    {
      std::ifstream nonceStorage(m_settings.nonceStorageFile.string().c_str());
      if (!nonceStorage) {
        GetTsLog().Debug("Failed to open %1% to read.",
                         m_settings.nonceStorageFile);
      } else {
        nonceStorage.read(reinterpret_cast<char *>(&nextNonce),
                          sizeof(nextNonce));
        GetTsLog().Debug("Restored nonce-value %1% from %2%.",
                         nextNonce,                     // 1
                         m_settings.nonceStorageFile);  // 2
      }
      if (nextNonce < m_settings.initialNonce) {
        GetTsLog().Debug("Using initial nonce-value %1%.",
                         m_settings.initialNonce);
        nextNonce = m_settings.initialNonce;
      }
      m_nonceStorage =
          std::ofstream(m_settings.nonceStorageFile.string().c_str(),
                        std::fstream::out | std::fstream::binary);
      if (!m_nonceStorage) {
        GetTsLog().Error("Failed to open %1% to store.",
                         m_settings.nonceStorageFile);
      }
    }

    std::vector<std::string> funds;
    std::vector<std::string> fundsInclOrders;
    std::vector<std::string> rights;
    size_t numberOfTransactions = 0;
    size_t numberOfActiveOrders = 0;

    TradeRequest request("getInfo", nextNonce++, m_settings);
    StoreNextNonce(nextNonce);
    try {
      const auto response =
          boost::get<1>(request.Send(m_marketDataSession, GetContext()));
      MakeServerAnswerDebugDump(response, *this);
      {
        const auto fundsNode = response.get_child_optional("funds");
        if (fundsNode) {
          for (const auto &node : *fundsNode) {
            funds.emplace_back(node.first + ": " + node.second.data());
          }
        }
      }
      {
        const auto &fundsInclOrdersNode =
            response.get_child_optional("funds_incl_orders");
        if (fundsInclOrdersNode) {
          for (const auto &node : *fundsInclOrdersNode) {
            fundsInclOrders.emplace_back(node.first + ": " +
                                         node.second.data());
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
      GetTsLog().Error(
          "Failed to read general account information: \"%1%\". Please check "
          "YoBit.Net credential, initial nonce-value in settings and/or "
          "nonce-value storage file %2%.",
          ex.what(),                     // 1
          m_settings.nonceStorageFile);  // 2
    }

    GetTsLog().Info(
        "Funds: %1%. Funds incl. order: %2%. Rights: %3%. Number of "
        "transactions: %4%. Number of active orders: %5%.",
        funds.empty() ? "none" : boost::join(funds, ", "),  // 1
        fundsInclOrders.empty() ? "none"
                                : boost::join(fundsInclOrders, ", "),  // 2
        rights.empty() ? "none" : boost::join(rights, ", "),           // 3
        numberOfTransactions,                                          // 4
        numberOfActiveOrders);                                         // 5

    RequestOpenedOrders();
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
    {
      Verify(
          m_securities
              .emplace(NormilizeSymbol(result->GetSymbol().GetSymbol()), result)
              .second);
    }

    return *result;
  }

  virtual std::unique_ptr<TransactionContext> SendOrderTransaction(
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

    boost::format requestParams("pair=%1%&type=%2%&rate=%3$.8f&amount=%4$.8f");
    requestParams % NormilizeSymbol(security.GetSymbol().GetSymbol())  // 1
        % (side == ORDER_SIDE_SELL ? "sell" : "buy")                   // 2
#ifdef _DEBUG
        % (side == ORDER_SIDE_SELL ? *price * 1.1 : *price * 0.9)  // 3
#else
        % *price  // 3
#endif
        % qty;  // 4
    TradeRequest request("Trade", m_nextNonce++, m_settings,
                         requestParams.str());
    StoreNextNonce(m_nextNonce);
    const auto &result = request.Send(m_tradingSession, GetContext());
    MakeServerAnswerDebugDump(boost::get<1>(result), *this);

    try {
      return boost::make_unique<TransactionContext>(
          boost::get<1>(result).get<OrderId>("order_id"));
    } catch (const std::exception &ex) {
      boost::format error("Failed to read order transaction reply: \"%1%\"");
      error % ex.what();
      throw TradingSystem::Error(error.str().c_str());
    }
  }

  virtual void SendCancelOrderTransaction(const OrderId &orderId) override {
    TradeRequest request(
        "CancelOrder", m_nextNonce++, m_settings,
        "order_id=" + boost::lexical_cast<std::string>(orderId));
    try {
      StoreNextNonce(m_nextNonce);
      const auto orders =
          boost::get<1>(request.Send(m_tradingSession, GetContext()));
      MakeServerAnswerDebugDump(orders, *this);
    } catch (const OrderIsUnknown &) {
      GetContext().GetTimer().Schedule([this] { RequestOpenedOrders(); },
                                       m_timerScope);
      throw;
    }

    GetContext().GetTimer().Schedule([this] { RequestOpenedOrders(); },
                                     m_timerScope);
  }

 private:
  void StoreNextNonce(size_t nextNonce) {
    m_nonceStorage.seekp(0);
    m_nonceStorage.write(reinterpret_cast<const char *>(&nextNonce),
                         sizeof(nextNonce));
    m_nonceStorage.flush();
    AssertEq(sizeof(nextNonce), m_nonceStorage.tellp());
    m_nextNonce = nextNonce;
  }

  void UpdateOrder(const Order &order, const OrderStatus &status) {
    OnOrder(order.id, order.tradingSystemOrderId, order.symbol, status,
            order.qty, order.qty, order.price, order.side, order.tid,
            order.time, order.time);
  }

  void RequestOpenedOrders() {
    for (const auto &symbol : m_settings.defaultSymbols) {
      RequestOpenedOrders(symbol);
    }
  }

  void RequestOpenedOrders(const std::string &symbol) {
    TradeRequest request("ActiveOrders", m_nextNonce++, m_settings,
                         "pair=" + NormilizeSymbol(symbol));
    StoreNextNonce(m_nextNonce);
    boost::unordered_map<OrderId, Order> notifiedOrderOrders;
    try {
      {
        const auto orders =
            boost::get<1>(request.Send(m_tradingSession, GetContext()));
        MakeServerAnswerDebugDump(orders, *this);
        for (const auto &orderNode : orders) {
          const auto &order = orderNode.second;
          const std::string &orderId = orderNode.first;

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

          const Order notifiedOrder = {boost::lexical_cast<OrderId>(orderId),
                                       orderId,
                                       std::move(symbol),
                                       std::move(qty),
                                       Price(order.get<double>("rate")),
                                       std::move(side),
                                       TIME_IN_FORCE_GTC,
                                       time};
          UpdateOrder(notifiedOrder, ORDER_STATUS_SUBMITTED);
          m_orders.erase(notifiedOrder.id);
          {
            const auto id = notifiedOrder.id;
            notifiedOrderOrders.emplace(id, std::move(notifiedOrder));
          }
        }
      }
      for (const auto &canceledOrder : m_orders) {
        UpdateOrder(canceledOrder.second, ORDER_STATUS_CANCELLED);
      }
      m_orders.swap(notifiedOrderOrders);
    } catch (const std::exception &ex) {
      GetTsLog().Error("Failed to request order list for \"%1%\": \"%2%\".",
                       symbol,      // 1
                       ex.what());  // 2
    }
  }

  void UpdateSecuritues(PublicRequest &depthRequest) {
    const auto &response = depthRequest.Send(m_marketDataSession, GetContext());
    const auto &time = boost::get<0>(response);
    const auto &delayMeasurement = boost::get<2>(response);
    try {
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
      }
    } catch (const std::exception &ex) {
      boost::format error("Failed to read \"depth\": \"%1%\"");
      error % ex.what();
      throw MarketDataSource::Error(error.str().c_str());
    }
  }

  void UpdateOrders() {
    for (const OrderId &orderId : GetActiveOrderList()) {
      TradeRequest request(
          "OrderInfo", m_nextNonce++, m_settings,
          "order_id=" + boost::lexical_cast<std::string>(orderId));
      StoreNextNonce(m_nextNonce);
      try {
        const auto response =
            boost::get<1>(request.Send(m_tradingSession, GetContext()));
        MakeServerAnswerDebugDump(response, *this);
        for (const auto &node : response) {
          const auto &order = node.second;

          const auto &time =
              pt::from_time_t(order.get<time_t>("timestamp_created"));

          const Qty qty = order.get<double>("start_amount");
          const Qty remainingQty = order.get<double>("amount");

          OrderStatus status;
          switch (order.get<int>("status")) {
            case 0:
              status = qty != remainingQty ? ORDER_STATUS_FILLED_PARTIALLY
                                           : ORDER_STATUS_SUBMITTED;
              break;
            case 1:
              status = ORDER_STATUS_FILLED;
              break;
            case 2:
            case 3:
              status = ORDER_STATUS_CANCELLED;
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

          OnOrder(orderId, boost::lexical_cast<std::string>(orderId),
                  boost::to_upper_copy(order.get<std::string>("pair")), status,
                  qty, remainingQty, Price(order.get<double>("rate")), side,
                  TIME_IN_FORCE_GTC, time, time);
        }
      } catch (const std::exception &ex) {
        GetTsLog().Error("Failed to request state for order %1%: \"%2%\".",
                         orderId, ex.what());
        throw Exception("Failed to request order state");
      }
    }
  }

 private:
  Settings m_settings;

  net::HTTPSClientSession m_marketDataSession;
  net::HTTPSClientSession m_tradingSession;

  size_t m_nextNonce;
  std::ofstream m_nonceStorage;

  boost::unordered_map<std::string, boost::shared_ptr<Rest::Security>>
      m_securities;

  std::unique_ptr<PollingTask> m_pollingTask;

  boost::unordered_map<OrderId, Order> m_orders;

  trdk::Timer::Scope m_timerScope;
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