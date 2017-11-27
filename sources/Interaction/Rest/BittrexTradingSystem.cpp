/*******************************************************************************
 *   Created: 2017/11/18 13:18:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BittrexTradingSystem.hpp"
#include "BittrexRequest.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

BittrexTradingSystem::Settings::Settings(const IniSectionRef &conf,
                                         ModuleEventsLog &log)
    : Base(conf, log),
      apiKey(conf.ReadKey("api_key")),
      apiSecret(conf.ReadKey("api_secret")) {
  log.Info("API key: \"%1%\". API secret: %2%.",
           apiKey,                                     // 1
           apiSecret.empty() ? "not set" : "is set");  // 2
}

////////////////////////////////////////////////////////////////////////////////

class BittrexTradingSystem::PrivateRequest : public BittrexRequest {
 public:
  typedef BittrexRequest Base;

 public:
  explicit PrivateRequest(const std::string &name,
                          const std::string &uriParams,
                          const Settings &settings)
      : Base(name,
             name,
             AppendUriParams("apikey=" + settings.apiKey, uriParams)),
        m_settings(settings) {}
  virtual ~PrivateRequest() override = default;

 protected:
  virtual void PrepareRequest(const net::HTTPClientSession &session,
                              const std::string &body,
                              net::HTTPRequest &request) const override {
    using namespace trdk::Lib::Crypto;
    const auto &digest =
        Hmac::CalcSha512Digest((session.secure() ? "https://" : "http://") +
                                   session.getHost() + GetRequest().getURI(),
                               m_settings.apiSecret);
    request.set("apisign", EncodeToHex(&digest[0], digest.size()));
    Base::PrepareRequest(session, body, request);
  }

 private:
  const Settings &m_settings;
};

class BittrexTradingSystem::OrderTransactionRequest : public PrivateRequest {
 public:
  typedef PrivateRequest Base;

 public:
  explicit OrderTransactionRequest(const std::string &name,
                                   const std::string &uriParams,
                                   const Settings &settings)
      : Base(name, uriParams, settings) {}
  virtual ~OrderTransactionRequest() override = default;

 protected:
  virtual bool IsPriority() const override { return true; }
};

class BittrexTradingSystem::NewOrderRequest : public OrderTransactionRequest {
 public:
  typedef OrderTransactionRequest Base;

 public:
  explicit NewOrderRequest(const std::string &name,
                           const std::string &productId,
                           const Qty &qty,
                           const Price &price,
                           const Settings &settings)
      : Base(name, CreateUriParams(productId, qty, price), settings) {}

 public:
  OrderId SendOrderTransaction(net::HTTPClientSession &session,
                               const Context &context) {
    const auto response = boost::get<1>(Base::Send(session, context));
    return response.get<std::string>("uuid");
  }

 private:
  static std::string CreateUriParams(const std::string &productId,
                                     const Qty &qty,
                                     const Price &price) {
    boost::format result("market=%1%&quantity=%2$.8f&rate=%3$.8f");
    result % productId  // 1
        % qty           // 2
        % price;        // 3
    return result.str();
  }
};

class BittrexTradingSystem::SellOrderRequest : public NewOrderRequest {
 public:
  typedef NewOrderRequest Base;

 public:
  explicit SellOrderRequest(const std::string &symbol,
                            const Qty &qty,
                            const Price &price,
                            const Settings &settings)
      : Base("/market/selllimit", symbol, qty, price, settings) {}
};

class BittrexTradingSystem::BuyOrderRequest : public NewOrderRequest {
 public:
  typedef NewOrderRequest Base;

 public:
  explicit BuyOrderRequest(const std::string &symbol,
                           const Qty &qty,
                           const Price &price,
                           const Settings &settings)
      : Base("/market/buylimit", symbol, qty, price, settings) {}
};

class BittrexTradingSystem::OrderCancelRequest : public PrivateRequest {
 public:
  typedef PrivateRequest Base;

 public:
  explicit OrderCancelRequest(const OrderId &id, const Settings &settings)
      : Base("/market/cancel", "uuid=" + id.GetValue(), settings) {}
  virtual ~OrderCancelRequest() override = default;

 protected:
  virtual bool IsPriority() const override { return false; }
};

class BittrexTradingSystem::AccountRequest : public PrivateRequest {
 public:
  typedef PrivateRequest Base;

 public:
  explicit AccountRequest(const std::string &name,
                          const std::string &uriParams,
                          const Settings &settings)
      : Base(name, uriParams, settings) {}
  virtual ~AccountRequest() override = default;

 protected:
  virtual bool IsPriority() const override { return false; }
};

class BittrexTradingSystem::OrderStateRequest : public AccountRequest {
 public:
  typedef AccountRequest Base;

 public:
  explicit OrderStateRequest(const OrderId &id, const Settings &settings)
      : Base("/account/getorder", "uuid=" + id.GetValue(), settings) {}
};

class BittrexTradingSystem::OrderHistoryRequest : public AccountRequest {
 public:
  typedef AccountRequest Base;

 public:
  explicit OrderHistoryRequest(const Settings &settings)
      : Base("/account/getorderhistory", std::string(), settings) {}
};

class BittrexTradingSystem::BalanceRequest : public AccountRequest {
 public:
  typedef AccountRequest Base;

 public:
  explicit BalanceRequest(const Settings &settings)
      : Base("/account/getbalances", std::string(), settings) {}
};

////////////////////////////////////////////////////////////////////////////////

BittrexTradingSystem::BittrexTradingSystem(const TradingMode &mode,
                                           Context &context,
                                           const std::string &instanceName,
                                           const IniSectionRef &conf)
    : Base(mode, context, instanceName),
      m_settings(conf, GetLog()),
      m_isConnected(false),
      m_tradingSession("bittrex.com"),
      m_ordersSession("bittrex.com"),
      m_pullingTask(pt::seconds(1), GetLog()) {}

void BittrexTradingSystem::CreateConnection(const IniSectionRef &) {
  Assert(!m_isConnected);

  try {
    RequestBalance();
    m_products =
        RequestBittrexProductList(m_tradingSession, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  Verify(m_pullingTask.AddTask("Actual orders", 0,
                               [this]() {
                                 UpdateOrders();
                                 return true;
                               },
                               1));

  m_isConnected = true;
}

std::unique_ptr<OrderTransactionContext>
BittrexTradingSystem::SendOrderTransaction(trdk::Security &security,
                                           const Currency &currency,
                                           const Qty &qty,
                                           const boost::optional<Price> &price,
                                           const OrderParams &params,
                                           const OrderSide &side,
                                           const TimeInForce &tif) {
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

  const auto &id = product->second.id;
  const auto &actualPrice = *price;

  return boost::make_unique<OrderTransactionContext>(
      side == ORDER_SIDE_BUY
          ? BuyOrderRequest(id, qty, actualPrice, m_settings)
                .SendOrderTransaction(m_tradingSession, GetContext())
          : SellOrderRequest(id, qty, actualPrice, m_settings)
                .SendOrderTransaction(m_tradingSession, GetContext()));
}

void BittrexTradingSystem::SendCancelOrderTransaction(const OrderId &orderId) {
  OrderCancelRequest(orderId, m_settings).Send(m_tradingSession, GetContext());
}

void BittrexTradingSystem::RequestBalance() {
  const auto responce =
      BalanceRequest(m_settings).Send(m_tradingSession, GetContext());
  for (const auto &node : boost::get<1>(responce)) {
    const auto &balance = node.second;
    GetLog().Info(
        "\"%1%\" balance: %2$.8f (available: %3$.8f, pending %4$.8f).",
        balance.get<std::string>("Currency"),  // 1
        balance.get<double>("Balance"),        // 2
        balance.get<double>("Available"),      // 3
        balance.get<double>("Pending"));       // 4
  }
}

namespace {
void RestoreCurrency(std::string &currency) {
  //! Also see symbol normalization.
  if (currency == "BCC") {
    currency = "BCH";
  }
}
std::string RestoreSymbol(const std::string &source) {
  std::vector<std::string> subs;
  boost::split(subs, source, boost::is_any_of("-"));
  AssertEq(2, subs.size());
  if (subs.size() >= 2) {
    subs[0].swap(subs[1]);
  }
  RestoreCurrency(subs[0]);
  RestoreCurrency(subs[1]);
  return boost::join(subs, "-");
}
pt::ptime ParseTime(std::string &&source) {
  AssertLe(19, source.size());
  AssertEq('T', source[10]);
  if (source.size() < 11 || source[10] != 'T') {
    return pt::not_a_date_time;
  }
  source[10] = ' ';
  return pt::time_from_string(std::move(source));
}
}

void BittrexTradingSystem::UpdateOrder(const ptr::ptree &order) {
  OrderSide side;
  {
    const auto &typeField = order.get<std::string>("Type");
    if (typeField == "LIMIT_BUY") {
      side = ORDER_SIDE_BUY;
    } else if (typeField == "LIMIT_SELL") {
      side = ORDER_SIDE_SELL;
    } else {
      GetLog().Error("Unknown order type \"%1%\".", typeField);
      return;
    }
  }

  const Price price = order.get<double>("Limit");
  AssertLt(0, price);

  const Qty qty = order.get<double>("Quantity");
  const Qty remainingQty = order.get<double>("QuantityRemaining");
  AssertGe(qty, remainingQty);

  OrderStatus status;
  if (order.get<bool>("CancelInitiated")) {
    status = ORDER_STATUS_CANCELLED;
  } else if (order.get<bool>("IsOpen")) {
    status = remainingQty < qty ? ORDER_STATUS_FILLED_PARTIALLY
                                : ORDER_STATUS_SUBMITTED;
  } else {
    status = ORDER_STATUS_FILLED;
  }

  const auto &openTime = ParseTime(order.get<std::string>("Opened"));
  if (openTime == pt::not_a_date_time) {
    GetLog().Error("Failed to parse order opening time \"%1%\".",
                   order.get<std::string>("Opened"));
    return;
  }
  pt::ptime closeTime;
  {
    auto closeTimeField = order.get_optional<std::string>("Opened");
    if (closeTimeField && !closeTimeField->empty()) {
      closeTime = ParseTime(std::move(*closeTimeField));
      if (closeTime == pt::not_a_date_time) {
        GetLog().Error("Failed to parse order closing time \"%1%\".",
                       *closeTimeField);
      }
    }
  }

  OnOrder(order.get<std::string>("OrderUuid"),
          RestoreSymbol(order.get<std::string>("Exchange")), std::move(status),
          std::move(qty), std::move(remainingQty), std::move(price),
          std::move(side), TIME_IN_FORCE_GTC, std::move(openTime),
          std::move(closeTime));
}

void BittrexTradingSystem::UpdateOrders() {
  for (const OrderId &orderId : GetActiveOrderList()) {
    OrderStateRequest request(orderId, m_settings);
    try {
      UpdateOrder(boost::get<1>(request.Send(m_ordersSession, GetContext())));
    } catch (const OrderIsUnknown &) {
      OnOrderCancel(orderId);
    } catch (const std::exception &ex) {
      GetLog().Error("Failed to update order list: \"%1%\".", ex.what());
      throw Exception("Failed to update order list");
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateBittrexTradingSystem(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<BittrexTradingSystem>(
      mode, context, instanceName, configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
