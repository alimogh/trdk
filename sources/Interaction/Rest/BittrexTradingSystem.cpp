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
#include "Util.hpp"

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

BittrexTradingSystem::PrivateRequest::PrivateRequest(
    const std::string &name,
    const std::string &uriParams,
    const Settings &settings,
    const Context &context,
    ModuleEventsLog &log,
    ModuleTradingLog *tradingLog)
    : Base(name,
           name,
           AppendUriParams("apikey=" + settings.apiKey, uriParams),
           context,
           log,
           tradingLog),
      m_settings(settings) {}

void BittrexTradingSystem::PrivateRequest::PrepareRequest(
    const net::HTTPClientSession &session,
    const std::string &body,
    net::HTTPRequest &request) const {
  using namespace trdk::Lib::Crypto;
  const auto &digest =
      Hmac::CalcSha512Digest((session.secure() ? "https://" : "http://") +
                                 session.getHost() + GetRequest().getURI(),
                             m_settings.apiSecret);
  request.set("apisign", EncodeToHex(&digest[0], digest.size()));
  Base::PrepareRequest(session, body, request);
}

class BittrexTradingSystem::OrderTransactionRequest : public PrivateRequest {
 public:
  typedef PrivateRequest Base;

 public:
  explicit OrderTransactionRequest(const std::string &name,
                                   const std::string &uriParams,
                                   const Settings &settings,
                                   const Context &context,
                                   ModuleEventsLog &log,
                                   ModuleTradingLog &tradingLog)
      : Base(name, uriParams, settings, context, log, &tradingLog) {}
  virtual ~OrderTransactionRequest() override = default;

 protected:
  virtual bool IsPriority() const override { return true; }
};

////////////////////////////////////////////////////////////////////////////////

BittrexTradingSystem::BittrexTradingSystem(const App &,
                                           const TradingMode &mode,
                                           Context &context,
                                           const std::string &instanceName,
                                           const IniSectionRef &conf)
    : Base(mode, context, instanceName),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_balances(GetLog(), GetTradingLog()),
      m_balancesRequest(m_settings, GetContext(), GetLog()),
      m_tradingSession(CreateSession("bittrex.com", m_settings, true)),
      m_pullingSession(CreateSession("bittrex.com", m_settings, false)),
      m_pullingTask(m_settings.pullingSetttings, GetLog()) {}

void BittrexTradingSystem::CreateConnection(const IniSectionRef &) {
  Assert(!IsConnected());

  try {
    UpdateBalances();
    m_products =
        RequestBittrexProductList(*m_tradingSession, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  m_pullingTask.AddTask(
      "Actual orders", 0,
      [this]() {
        UpdateOrders();
        return true;
      },
      m_settings.pullingSetttings.GetActualOrdersRequestFrequency(), false);
  m_pullingTask.AddTask(
      "Balances", 1,
      [this]() {
        UpdateBalances();
        return true;
      },
      m_settings.pullingSetttings.GetBalancesRequestFrequency(), true);

  m_pullingTask.AccelerateNextPulling();
}

boost::optional<BittrexTradingSystem::OrderCheckError>
BittrexTradingSystem::CheckOrder(const trdk::Security &security,
                                 const Currency &currency,
                                 const Qty &qty,
                                 const boost::optional<Price> &price,
                                 const OrderSide &side) const {
  {
    const auto &result = Base::CheckOrder(security, currency, qty, price, side);
    if (result) {
      return result;
    }
  }
  const auto &symbol = security.GetSymbol();
  if (symbol.GetQuoteSymbol() == "BTC") {
    const auto minVolume = 0.001;
    if (price && qty * *price < minVolume) {
      return OrderCheckError{boost::none, boost::none, minVolume};
    }
    if (symbol.GetBaseSymbol() == "ETH") {
      const auto minQty = 0.015;
      if (qty < minQty) {
        return OrderCheckError{minQty};
      }
    }
  } else if (symbol.GetQuoteSymbol() == "ETH") {
    const auto minVolume = 0.0005;
    if (price && qty * *price < minVolume) {
      return OrderCheckError{boost::none, boost::none, minVolume};
    }
    if (symbol.GetBaseSymbol() == "LTC") {
      const auto minQty = 0.06;
      if (qty < minQty) {
        return OrderCheckError{minQty};
      }
    }
  }
  return boost::none;
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
      throw Exception("Order time-in-force type is not supported");
  }
  if (currency != security.GetSymbol().GetCurrency()) {
    throw Exception("Trading system supports only security quote currency");
  }
  if (!price) {
    throw Exception("Market order is not supported");
  }

  const auto &product = m_products.find(security.GetSymbol().GetSymbol());
  if (product == m_products.cend()) {
    throw Exception("Symbol is not supported by exchange");
  }

  const auto &productId = product->second.id;
  const auto &actualPrice = *price;

  class NewOrderRequest : public OrderTransactionRequest {
   public:
    explicit NewOrderRequest(const std::string &name,
                             const std::string &productId,
                             const Qty &qty,
                             const Price &price,
                             const Settings &settings,
                             const Context &context,
                             ModuleEventsLog &log,
                             ModuleTradingLog &tradingLog)
        : OrderTransactionRequest(name,
                                  CreateUriParams(productId, qty, price),
                                  settings,
                                  context,
                                  log,
                                  tradingLog) {}

   public:
    OrderId SendOrderTransaction(net::HTTPClientSession &session) {
      const auto response = boost::get<1>(Base::Send(session));
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

  return boost::make_unique<OrderTransactionContext>(
      *this,
      NewOrderRequest(
          side == ORDER_SIDE_BUY ? "/market/buylimit" : "/market/selllimit",
          productId, qty, actualPrice, m_settings, GetContext(), GetLog(),
          GetTradingLog())
          .SendOrderTransaction(*m_tradingSession));
}

void BittrexTradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &transaction) {
  OrderTransactionRequest(
      "/market/cancel",
      "uuid=" + boost::lexical_cast<std::string>(transaction.GetOrderId()),
      m_settings, GetContext(), GetLog(), GetTradingLog())
      .Send(*m_tradingSession);
}

void BittrexTradingSystem::OnTransactionSent(
    const OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pullingTask.AccelerateNextPulling();
}

void BittrexTradingSystem::UpdateBalances() {
  const auto response = m_balancesRequest.Send(*m_pullingSession);
  for (const auto &node : boost::get<1>(response)) {
    const auto &balance = node.second;
    auto symbol = balance.get<std::string>("Currency");
    if (symbol == "BCC") {
      symbol = "BCH";
    }
    m_balances.SetAvailableToTrade(std::move(symbol),
                                   balance.get<Volume>("Available"));
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
}

void BittrexTradingSystem::UpdateOrder(const OrderId &orderId,
                                       const ptr::ptree &order) {
  OrderStatus status = ORDER_STATUS_ERROR;
  pt::ptime time;

  struct ExecInfo {
    Volume commission;
    Price tradePrice;
  };
  boost::optional<ExecInfo> execInfo;

  try {
    AssertEq(orderId, order.get<OrderId>("OrderUuid"));

    if (order.get<bool>("IsOpen")) {
      status = ORDER_STATUS_OPENED;
    } else {
      const auto &remainingQty = order.get<Qty>("QuantityRemaining");
      const auto &qty = order.get<Qty>("Quantity");
      if (qty != remainingQty) {
        AssertGe(qty, remainingQty);
        execInfo = ExecInfo{order.get<Volume>("CommissionPaid"),
                            order.get<Price>("PricePerUnit")};
        status = remainingQty || order.get<bool>("CancelInitiated")
                     ? ORDER_STATUS_CANCELED
                     : ORDER_STATUS_FILLED_FULLY;
      } else {
        Assert(order.get<bool>("CancelInitiated"));
        status = ORDER_STATUS_CANCELED;
      }
    }

    {
      auto closeTimeField = order.get_optional<std::string>("Opened");
      if (closeTimeField && !closeTimeField->empty()) {
        time = ParseTime(std::move(*closeTimeField));
        if (time == pt::not_a_date_time) {
          GetLog().Error("Failed to parse order closing time \"%1%\".",
                         *closeTimeField);
          Assert(time != pt::not_a_date_time);
          status = ORDER_STATUS_ERROR;
          time = GetContext().GetCurrentTime();
        }
      } else {
        time = ParseTime(order.get<std::string>("Opened"));
        if (time == pt::not_a_date_time) {
          GetLog().Error("Failed to parse order opening time \"%1%\".",
                         order.get<std::string>("Opened"));
          Assert(time != pt::not_a_date_time);
          status = ORDER_STATUS_ERROR;
          time = GetContext().GetCurrentTime();
        }
      }
    }
  } catch (const std::exception &ex) {
    boost::format error("Failed to read order: \"%1%\"");
    error % ex.what();
    throw Exception(error.str().c_str());
  }

  !execInfo ? OnOrderStatusUpdate(time, orderId, status)
            : OnOrderStatusUpdate(time, orderId, status,
                                  Trade{std::move(execInfo->tradePrice)},
                                  execInfo->commission);
}

void BittrexTradingSystem::UpdateOrders() {
  for (const OrderId &orderId : GetActiveOrderIdList()) {
    AccountRequest request("/account/getorder", "uuid=" + orderId.GetValue(),
                           m_settings, GetContext(), GetLog(),
                           &GetTradingLog());
    UpdateOrder(orderId, boost::get<1>(request.Send(*m_pullingSession)));
  }
}

pt::ptime BittrexTradingSystem::ParseTime(std::string &&source) const {
  AssertLe(19, source.size());
  AssertEq('T', source[10]);
  if (source.size() < 11 || source[10] != 'T') {
    return pt::not_a_date_time;
  }
  source[10] = ' ';
  return pt::time_from_string(std::move(source)) - m_serverTimeDiff;
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateBittrexTradingSystem(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<BittrexTradingSystem>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
