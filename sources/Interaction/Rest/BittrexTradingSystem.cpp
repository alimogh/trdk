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
using namespace Lib;
using namespace Interaction::Rest;
namespace net = Poco::Net;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

BittrexTradingSystem::Settings::Settings(const ptr::ptree &conf,
                                         ModuleEventsLog &log)
    : Base(conf, log),
      apiKey(conf.get<std::string>("config.auth.apiKey")),
      apiSecret(conf.get<std::string>("config.auth.apiSecret")) {
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
  using namespace Crypto;
  const auto &digest =
      Hmac::CalcSha512Digest((session.secure() ? "https://" : "http://") +
                                 session.getHost() + GetRequest().getURI(),
                             m_settings.apiSecret);
  request.set("apisign", EncodeToHex(&digest[0], digest.size()));
  Base::PrepareRequest(session, body, request);
}

void BittrexTradingSystem::PrivateRequest::WriteUri(
    std::string uri, net::HTTPRequest &request) const {
  Base::WriteUri(
      uri + "?nonce=" + to_iso_string(pt::microsec_clock::universal_time()),
      request);
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
  ~OrderTransactionRequest() override = default;

 protected:
  bool IsPriority() const override { return true; }
};

////////////////////////////////////////////////////////////////////////////////

BittrexTradingSystem::BittrexTradingSystem(const App &,
                                           const TradingMode &mode,
                                           Context &context,
                                           std::string instanceName,
                                           std::string title,
                                           const ptr::ptree &conf)
    : Base(mode, context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_balancesRequest(m_settings, GetContext(), GetLog()),
      m_tradingSession(CreateSession("bittrex.com", m_settings, true)),
      m_pollingSession(CreateSession("bittrex.com", m_settings, false)),
      m_pollingTask(m_settings.pollingSetttings, GetLog()) {}

void BittrexTradingSystem::CreateConnection() {
  Assert(!IsConnected());

  try {
    UpdateBalances();
    m_products =
        RequestBittrexProductList(m_tradingSession, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  m_pollingTask.AddTask(
      "Actual orders", 0,
      [this]() {
        UpdateOrders();
        return true;
      },
      m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
  m_pollingTask.AddTask(
      "Balances", 1,
      [this]() {
        UpdateBalances();
        return true;
      },
      m_settings.pollingSetttings.GetBalancesRequestFrequency(), true);

  m_pollingTask.AccelerateNextPolling();
}

boost::optional<OrderCheckError> BittrexTradingSystem::CheckOrder(
    const trdk::Security &security,
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
  const auto &product = m_products.find(security.GetSymbol().GetSymbol());
  if (product == m_products.cend()) {
    GetLog().Error(R"(Failed find product for "%1%" to check order.)",
                   security);
    throw Exception("Symbol is not supported by exchange");
  }
  boost::optional<OrderCheckError> result;
  if (qty < product->second.minQty) {
    result.emplace(OrderCheckError{product->second.minQty});
  }
  if (price) {
    // DUST_TRADE_DISALLOWED_MIN_VALUE_50K_SAT, see
    // https://support.bittrex.com/hc/en-us/articles/115000240791-Error-Codes-Troubleshooting-common-error-codes
    const auto minVolume = 0.0005;
    if (*price * qty < 0.0005) {
      if (!result) {
        result.emplace(OrderCheckError{boost::none, boost::none, minVolume});
      } else {
        result->volume = minVolume;
      }
    }
  }

  return result;
}

bool BittrexTradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products.count(symbol) > 0;
}

Volume BittrexTradingSystem::CalcCommission(const Qty &qty,
                                            const Price &price,
                                            const OrderSide &,
                                            const trdk::Security &) const {
  return (qty * price) * (0.25 / 100);
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
    OrderId SendOrderTransaction(
        std::unique_ptr<net::HTTPSClientSession> &session) {
      const auto response = boost::get<1>(Base::Send(session));
      try {
        return response.get<std::string>("uuid");
      } catch (const ptr::ptree_error &ex) {
        boost::format error(
            "Wrong server response to the request \"%1%\" (%2%): \"%3%\"");
        error % GetName()            // 1
            % GetRequest().getURI()  // 2
            % ex.what();             // 3
        throw Exception(error.str().c_str());
      }
    }

   private:
    static std::string CreateUriParams(const std::string &productId,
                                       const Qty &qty,
                                       const Price &price) {
      boost::format result("market=%1%&quantity=%2%&rate=%3%");
      result % productId  // 1
          % qty           // 2
          % price;        // 3
      return result.str();
    }
  };

  return boost::make_unique<OrderTransactionContext>(
      *this, NewOrderRequest(side == ORDER_SIDE_BUY ? "/market/buylimit"
                                                    : "/market/selllimit",
                             productId, qty, actualPrice, m_settings,
                             GetContext(), GetLog(), GetTradingLog())
                 .SendOrderTransaction(m_tradingSession));
}

void BittrexTradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &transaction) {
  OrderTransactionRequest(
      "/market/cancel",
      "uuid=" + boost::lexical_cast<std::string>(transaction.GetOrderId()),
      m_settings, GetContext(), GetLog(), GetTradingLog())
      .Send(m_tradingSession);
}

void BittrexTradingSystem::OnTransactionSent(
    const OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pollingTask.AccelerateNextPolling();
}

void BittrexTradingSystem::UpdateBalances() {
  const auto response = m_balancesRequest.Send(m_pollingSession);
  for (const auto &node : boost::get<1>(response)) {
    const auto &balanceNode = node.second;
    auto symbol = balanceNode.get<std::string>("Currency");
    if (symbol == "BCC") {
      symbol.back() = 'H';
    }
    const auto &balance = balanceNode.get<Volume>("Balance");
    auto available = balanceNode.get<Volume>("Available");
    AssertLe(available, balance);
    auto locked = balance - available;
    m_balances.Set(symbol, std::move(available), std::move(locked));
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
}  // namespace

void BittrexTradingSystem::UpdateOrder(const OrderId &orderId,
                                       const ptr::ptree &order) {
  try {
    AssertEq(orderId, order.get<OrderId>("OrderUuid"));

    pt::ptime time;
    {
      auto closeTimeField = order.get_optional<std::string>("Closed");
      if (closeTimeField && *closeTimeField != "null") {
        time = ParseTime(std::move(*closeTimeField));
        if (time == pt::not_a_date_time) {
          OnOrderError(GetContext().GetCurrentTime(), orderId, boost::none,
                       boost::none, "Failed to parse order closing time");
          return;
        }
      } else {
        time = ParseTime(order.get<std::string>("Opened"));
        if (time == pt::not_a_date_time) {
          OnOrderError(GetContext().GetCurrentTime(), orderId, boost::none,
                       boost::none, "Failed to parse order opening time");
          return;
        }
      }
    }

    if (order.get<bool>("IsOpen")) {
      OnOrderOpened(time, orderId);
    } else {
      const auto &remainingQty = order.get<Qty>("QuantityRemaining");
      const auto &qty = order.get<Qty>("Quantity");
      const auto &commission = order.get<Volume>("CommissionPaid");
      if (qty != remainingQty) {
        Trade trade = {order.get<Price>("PricePerUnit"), qty - remainingQty};
        if (remainingQty == 0) {
          OnOrderFilled(time, orderId, std::move(trade), commission);
        } else {
          OnTrade(time, orderId, std::move(trade));
          OnOrderCanceled(time, orderId, boost::none, commission);
        }
      } else {
        OnOrderCanceled(time, orderId, boost::none, commission);
      }
    }
  } catch (const Exception &ex) {
    boost::format error("Failed to apply order update: \"%1%\"");
    error % ex.what();
    throw Exception(error.str().c_str());
  } catch (const std::exception &ex) {
    boost::format error("Failed to read order: \"%1%\"");
    error % ex.what();
    throw Exception(error.str().c_str());
  }
}

void BittrexTradingSystem::UpdateOrders() {
  for (const auto &context : GetActiveOrderContextList()) {
    const auto &orderId = context->GetOrderId();
    AccountRequest request("/account/getorder", "uuid=" + orderId.GetValue(),
                           m_settings, GetContext(), GetLog(),
                           &GetTradingLog());
    UpdateOrder(orderId, boost::get<1>(request.Send(m_pollingSession)));
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
    std::string instanceName,
    std::string title,
    const ptr::ptree &configuration) {
  const auto &result = boost::make_shared<BittrexTradingSystem>(
      App::GetInstance(), mode, context, std::move(instanceName),
      std::move(title), configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
