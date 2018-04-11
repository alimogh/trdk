/*******************************************************************************
 *   Created: 2017/12/19 20:59:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "LivecoinTradingSystem.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Crypto;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace ptr = boost::property_tree;

////////////////////////////////////////////////////////////////////////////////

namespace {
class LivecoinOrderTransactionContext : public OrderTransactionContext {
 public:
  explicit LivecoinOrderTransactionContext(LivecoinTradingSystem &tradingSystem,
                                           const std::string &productRequestId,
                                           const OrderId &&orderId)
      : OrderTransactionContext(tradingSystem, std::move(orderId)),
        m_productRequestId(productRequestId) {}

 public:
  const std::string &GetProductRequestId() const { return m_productRequestId; }

 private:
  const std::string &m_productRequestId;
};
}  // namespace

////////////////////////////////////////////////////////////////////////////////

LivecoinTradingSystem::Settings::Settings(const IniSectionRef &conf,
                                          ModuleEventsLog &log)
    : Rest::Settings(conf, log),
      apiKey(conf.ReadKey("api_key")),
      apiSecret(conf.ReadKey("api_secret")) {
  log.Info("API key: \"%1%\". API secret: %2%.",
           apiKey,                                     // 1
           apiSecret.empty() ? "not set" : "is set");  // 2
}

////////////////////////////////////////////////////////////////////////////////

LivecoinTradingSystem::PrivateRequest::PrivateRequest(
    const std::string &name,
    const std::string &method,
    const Settings &settings,
    const std::string &params,
    const Context &context,
    ModuleEventsLog &log,
    ModuleTradingLog *tradingLog)
    : LivecoinRequest(name, method, params, context, log, tradingLog),
      m_settings(settings) {}

void LivecoinTradingSystem::PrivateRequest::PrepareRequest(
    const net::HTTPClientSession &session,
    const std::string &body,
    net::HTTPRequest &request) const {
  request.set("Api-key", m_settings.apiKey);
  {
    using namespace trdk::Lib::Crypto;
    const auto &digest =
        Hmac::CalcSha256Digest(GetUriParams(), m_settings.apiSecret);
    request.set("Sign",
                boost::to_upper_copy(EncodeToHex(&digest[0], digest.size())));
  }
  Base::PrepareRequest(session, body, request);
}

LivecoinTradingSystem::TradingRequest::TradingRequest(
    const std::string &name,
    const Settings &settings,
    const std::string &params,
    const Context &context,
    ModuleEventsLog &log,
    ModuleTradingLog *tradingLog)
    : PrivateRequest(name,
                     net::HTTPRequest::HTTP_POST,
                     settings,
                     params,
                     context,
                     log,
                     tradingLog) {}

LivecoinTradingSystem::TradingRequest::Response
LivecoinTradingSystem::TradingRequest::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  const auto &result = Base::Send(session);
  const auto &content = boost::get<1>(result);
  try {
    const auto &status = content.get<bool>("success");
    if (!status) {
      std::ostringstream error;
      error << "The server returned an error in response to the request \""
            << GetName() << "\" (" << GetRequest().getURI() << "): \"";
      const auto &exception = content.get<std::string>("exception");
      try {
        const auto &errorCode = boost::lexical_cast<int>(exception);
        switch (errorCode) {
          default:
          case 1:
            error << "Unknown error";
            break;
          case 2:
            error << "System error";
            break;
          case 10:
            error << "Authentication error";
            break;
          case 11:
            error << "Authentication is required";
            break;
          case 12:
            error << "Authentication failed";
            break;
          case 20:
            error << "Signature is not correct";
            break;
          case 30:
            error << "Access denied";
            break;
          case 31:
            error << "API disabled";
            break;
          case 32:
            error << "API restricted by IP";
            break;
          case 100:
            error << "Incorrect parameters";
            break;
          case 101:
            error << "Incorrect API key";
            break;
          case 102:
            error << "Incorrect User ID";
            break;
          case 103:
            error << "Incorrect currency";
            break;
          case 104:
            error << "Incorrect amount";
            break;
          case 150:
            error << "Unable to block funds";
            break;
        }
        error << "\" (error code: " << errorCode << ")";
      } catch (const boost::bad_lexical_cast &) {
        error << exception << "\"";
        if (boost::istarts_with(exception,
                                "Not sufficient funds on the account")) {
          throw InsufficientFundsException(error.str().c_str());
        } else if (boost::istarts_with(exception, "Cannot find order")) {
          throw OrderIsUnknownException(error.str().c_str());
        }
      }
      throw Exception(error.str().c_str());
    }
  } catch (const ptr::ptree_error &ex) {
    std::ostringstream error;
    error << "Failed to read server response for request \"" << GetName()
          << "\" (" << GetRequest().getURI() << "): \"" << ex.what() << "\"";
    throw CommunicationError(error.str().c_str());
  }
  return result;
}

LivecoinTradingSystem::BalancesRequest::BalancesRequest(
    const Settings &settings, const Context &context, ModuleEventsLog &log)
    : PrivateRequest("/payment/balances",
                     net::HTTPRequest::HTTP_GET,
                     settings,
                     std::string(),
                     context,
                     log) {}

////////////////////////////////////////////////////////////////////////////////

LivecoinTradingSystem::LivecoinTradingSystem(const App &,
                                             const TradingMode &mode,
                                             Context &context,
                                             const std::string &instanceName,
                                             const IniSectionRef &conf)
    : Base(mode, context, instanceName),
      m_settings(conf, GetLog()),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_balancesRequest(m_settings, GetContext(), GetLog()),
      m_tradingSession(CreateSession("api.livecoin.net", m_settings, true)),
      m_pollingSession(CreateSession("api.livecoin.net", m_settings, false)),
      m_pollingTask(m_settings.pollingSetttings, GetLog()) {}

void LivecoinTradingSystem::CreateConnection(const IniSectionRef &) {
  Assert(m_products.empty());

  try {
    UpdateBalances();
    m_products =
        RequestLivecoinProductList(m_tradingSession, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  m_pollingTask.AddTask(
      "Orders", 0,
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

Volume LivecoinTradingSystem::CalcCommission(const Qty &qty,
                                             const Price &price,
                                             const OrderSide &,
                                             const trdk::Security &) const {
  return (qty * price) * (0.18 / 100);
}

void LivecoinTradingSystem::UpdateBalances() {
  const auto response = boost::get<1>(m_balancesRequest.Send(m_pollingSession));
  for (const auto &node : response) {
    const auto &balance = node.second;
    const auto &type = balance.get<std::string>("type");
    if (type == "available") {
      m_balances.SetAvailableToTrade(balance.get<std::string>("currency"),
                                     balance.get<Volume>("value"));
    } else if (type == "trade") {
      m_balances.SetLocked(balance.get<std::string>("currency"),
                           balance.get<Volume>("value"));
    }
  }
}

void LivecoinTradingSystem::UpdateOrders() {
  class OrderStatusRequest : public PrivateRequest {
   public:
    typedef PrivateRequest Base;

   public:
    explicit OrderStatusRequest(const OrderId &orderId,
                                const Settings &settings,
                                const Context &context,
                                ModuleEventsLog &log,
                                ModuleTradingLog &tradingLog)
        : Base("/exchange/order",
               net::HTTPRequest::HTTP_GET,
               settings,
               "orderId=" + boost::lexical_cast<std::string>(orderId),
               context,
               log,
               &tradingLog) {}
    virtual ~OrderStatusRequest() override = default;

   protected:
    virtual bool IsPriority() const override { return false; }
  };

  for (const auto &context : GetActiveOrderContextList()) {
    const auto &orderId = context->GetOrderId();
    const auto response = OrderStatusRequest(orderId, m_settings, GetContext(),
                                             GetLog(), GetTradingLog())
                              .Send(m_pollingSession);
    const auto &time = boost::get<0>(response);
    const auto &order = boost::get<1>(response);
    try {
      const auto &remainingQty = order.get<Qty>("remaining_quantity");
      const auto &status = order.get<std::string>("status");
      if (status == "CANCELLED" || status == "PARTIALLY_FILLED_AND_CANCELLED") {
        const auto &qty = order.get<Qty>("quantity");
        AssertGe(qty, remainingQty);
        boost::optional<Volume> commission;
        if (qty != remainingQty) {
          commission = order.get<Volume>("trades.commission");
          OnTrade(
              time, orderId,
              Trade{order.get<Price>("trades.avg_price"), qty - remainingQty});
        } else {
          commission = order.get_optional<Volume>("trades.commission");
        }
        OnOrderCanceled(time, orderId, remainingQty, commission);
      } else if (status == "EXECUTED") {
        AssertEq(0, order.get<Price>("remaining_quantity"));
        OnOrderFilled(time, orderId,
                      Trade{order.get<Price>("trades.avg_price")},
                      order.get<Volume>("trades.commission"));
      } else if (status != "OPEN" && status != "PARTIALLY_FILLED") {
        OnOrderError(time, orderId, remainingQty,
                     order.get_optional<Volume>("trades.commission"),
                     "Unknown order status");
      } else {
        OnOrderOpened(time, orderId);
      }
    } catch (const Exception &ex) {
      std::ostringstream error;
      error << "Failed to apply order status update: \"" << ex.what() << "\"";
      throw Exception(error.str().c_str());
    } catch (const std::exception &ex) {
      std::ostringstream error;
      error << "Failed to read order status: \"" << ex.what() << "\"";
      throw Exception(error.str().c_str());
    }
  }
}

boost::optional<OrderCheckError> LivecoinTradingSystem::CheckOrder(
    const trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderSide &side) const {
  {
    const auto result = Base::CheckOrder(security, currency, qty, price, side);
    if (result) {
      return result;
    }
  }

  if (!price) {
    return boost::none;
  }

  const auto &productIt = m_products.find(security.GetSymbol().GetSymbol());
  if (productIt == m_products.cend()) {
    GetLog().Warn("Failed find product for \"%1%\" to check order.", security);
    return boost::none;
  }
  const auto &minBtcVolume = productIt->second.minBtcVolume;
  const auto &vol = *price * qty;

  const auto &symbol = security.GetSymbol();
  if (symbol.GetCurrency() == CURRENCY_BTC) {
    if (vol < minBtcVolume) {
      return OrderCheckError{boost::none, boost::none, minBtcVolume};
    }
  } else if (symbol.GetBaseSymbol() == "BTC") {
    if (qty < minBtcVolume) {
      return OrderCheckError{boost::none, boost::none, minBtcVolume};
    }
  } else {
    const auto &marketDataSource = GetContext().GetMarketDataSource(GetIndex());
    AssertEq(GetInstanceName(), marketDataSource.GetInstanceName());
    const auto *const btcSecurity = marketDataSource.FindSecurity(
        Symbol(security.GetSymbol().GetBaseSymbol() + "_BTC",
               SECURITY_TYPE_CRYPTO, CURRENCY_BTC));
    if (!btcSecurity) {
      GetLog().Warn(
          "Failed find BTC-security to check order volume for \"%1%\".",
          security);
      return boost::none;
    }
    if (vol * btcSecurity->GetAskPrice() < minBtcVolume) {
      return OrderCheckError{boost::none, boost::none,
                             minBtcVolume / btcSecurity->GetAskPrice()};
    }
  }
  return boost::none;
}

bool LivecoinTradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products.count(symbol) > 0;
}

std::unique_ptr<OrderTransactionContext>
LivecoinTradingSystem::SendOrderTransaction(trdk::Security &security,
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

  const auto &symbol = security.GetSymbol().GetSymbol();
  const auto &product = m_products.find(symbol);
  if (product == m_products.cend()) {
    throw Exception("Symbol is not supported by exchange");
  }

  std::ostringstream requestParams;
  requestParams << "currencyPair=" << product->second.requestId
                << "&price=" << std::fixed
                << std::setprecision(product->second.pricePrecision)
                << price->Get() << "&quantity=" << qty;

  TradingRequest request(
      side == ORDER_SIDE_BUY ? "/exchange/buylimit" : "/exchange/selllimit",
      m_settings, requestParams.str(), GetContext(), GetLog(),
      &GetTradingLog());
  const auto response = boost::get<1>(request.Send(m_tradingSession));
  try {
#if 0
  // "added" field has unknown semantic. Check temporary disabled.
    if (!response.get<bool>("added")) {
      throw Exception(
          ("Failed to add new order: " + ConvertToString(response, false))
              .c_str());
    }
#endif
    return boost::make_unique<LivecoinOrderTransactionContext>(
        *this, product->second.requestId, response.get<std::string>("orderId"));
  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        "Wrong server response to the request \"%1%\" (%2%): \"%3%\"");
    error % request.GetName()            // 1
        % request.GetRequest().getURI()  // 2
        % ex.what();                     // 3
    throw Exception(error.str().c_str());
  }
}

void LivecoinTradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &transaction) {
  boost::format requestParams("currencyPair=%1%&orderId=%2%");
  requestParams %
      boost::polymorphic_downcast<const LivecoinOrderTransactionContext *>(
          &transaction)
          ->GetProductRequestId()  // 1
      % transaction.GetOrderId();  // 2

  TradingRequest request("/exchange/cancellimit", m_settings,
                         requestParams.str(), GetContext(), GetLog(),
                         &GetTradingLog());
  const auto response = boost::get<1>(request.Send(m_tradingSession));

  try {
    if (!response.get<bool>("cancelled")) {
      boost::format error("Failed to cancel order: \"%1%\"");
      const auto &message = response.get<std::string>("message");
      error % message;
      boost::istarts_with(message,
                          "Failed to cancel order: can't find order") ||
              (boost::starts_with(message, "Order[orderId={") &&
               boost::contains(message, "}] isn't in OrderBook: status={"))
          ? throw OrderIsUnknownException(error.str().c_str())
          : throw Exception(error.str().c_str());
    }
  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        "Wrong server response to the request \"%1%\" (%2%): \"%3%\"");
    error % request.GetName()            // 1
        % request.GetRequest().getURI()  // 2
        % ex.what();                     // 3
    throw Exception(error.str().c_str());
  }
}

void LivecoinTradingSystem::OnTransactionSent(
    const OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pollingTask.AccelerateNextPolling();
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateLivecoinTradingSystem(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<LivecoinTradingSystem>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
