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

LivecoinTradingSystem::PrivateRequest::PrivateRequest(const std::string &name,
                                                      const std::string &method,
                                                      const Settings &settings,
                                                      const std::string &params)
    : LivecoinRequest(name, method, params), m_settings(settings) {}

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

LivecoinTradingSystem::TradingRequest::TradingRequest(const std::string &name,
                                                      const Settings &settings,
                                                      const std::string &params)
    : PrivateRequest(name, net::HTTPRequest::HTTP_POST, settings, params) {}

LivecoinTradingSystem::TradingRequest::Response
LivecoinTradingSystem::TradingRequest::Send(net::HTTPClientSession &session,
                                            const Context &context) {
  const auto &result = Base::Send(session, context);
  const auto &content = boost::get<1>(result);
  try {
    const auto &status = content.get<bool>("success");
    if (!status) {
      std::ostringstream error;
      error << "The server returned an error in response to the request \""
            << GetName() << "\" (" << GetRequest().getURI() << "): \"";
      const auto &exception = content.get<std::string>("exception");
      try {
        const auto &errorCode = boost::lexical_cast<int>("exception");
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
      }
      throw Exception(error.str().c_str());
    }
  } catch (const ptr::ptree_error &ex) {
    std::ostringstream error;
    error << "Field to read server response for request \"" << GetName()
          << "\" (" << GetRequest().getURI() << "): \"" << ex.what() << "\"";
    throw Interactor::CommunicationError(error.str().c_str());
  }
  return result;
}

LivecoinTradingSystem::BalancesRequest::BalancesRequest(
    const Settings &settings)
    : PrivateRequest(
          "/payment/balances", net::HTTPRequest::HTTP_GET, settings) {}

////////////////////////////////////////////////////////////////////////////////

LivecoinTradingSystem::LivecoinTradingSystem(const App &,
                                             const TradingMode &mode,
                                             Context &context,
                                             const std::string &instanceName,
                                             const IniSectionRef &conf)
    : Base(mode, context, instanceName),
      m_settings(conf, GetLog()),
      m_balances(GetLog(), GetTradingLog()),
      m_balancesRequest(m_settings),
      m_tradingSession(CreateSession("api.livecoin.net", m_settings, true)),
      m_pullingSession(CreateSession("api.livecoin.net", m_settings, false)),
      m_pullingTask(m_settings.pullingSetttings, GetLog()) {}

void LivecoinTradingSystem::CreateConnection(const IniSectionRef &) {
  Assert(m_products.empty());

  try {
    UpdateBalances();
    m_products =
        RequestLivecoinProductList(*m_tradingSession, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  m_pullingTask.AddTask(
      "Orders", 0,
      [this]() {
        UpdateOrders();
        return true;
      },
      m_settings.pullingSetttings.GetActualOrdersRequestFrequency());
  m_pullingTask.AddTask(
      "Balances", 1,
      [this]() {
        UpdateBalances();
        return true;
      },
      m_settings.pullingSetttings.GetBalancesRequestFrequency());

  m_pullingTask.AccelerateNextPulling();
}

Volume LivecoinTradingSystem::CalcCommission(const Volume &volume,
                                             const trdk::Security &) const {
  return volume * (0.18 / 100);
}

void LivecoinTradingSystem::UpdateBalances() {
  const auto response =
      boost::get<1>(m_balancesRequest.Send(*m_pullingSession, GetContext()));
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
                                const Settings &settings)
        : Base("/exchange/order",
               net::HTTPRequest::HTTP_GET,
               settings,
               "orderId=" + ExtractOrderId(orderId)) {}
    virtual ~OrderStatusRequest() override = default;

   public:
    virtual Response Send(net::HTTPClientSession &session,
                          const Context &context) override {
      return Base::Send(session, context);
    }

   protected:
    virtual bool IsPriority() const override { return false; }

   private:
    static std::string ExtractOrderId(const OrderId &source) {
      const auto &orderId = source.GetValue();
      const auto &pos = orderId.find('_');
      AssertNe(std::string::npos, pos);
      if (pos == std::string::npos) {
        return orderId;
      }
      return orderId.substr(pos + 1);
    }
  };

  for (const auto &orderId : GetActiveOrderList()) {
    const auto order =
        boost::get<1>(OrderStatusRequest(orderId, m_settings)
                          .Send(*m_pullingSession, GetContext()));
    GetTradingLog().Write("debug-dump-order-status\t%1%",
                          [&order](TradingRecord &record) {
                            record % ConvertToString(order, false);
                          });
    OrderStatus status;
    Qty remainingQuantity;
    Volume commission;
    TradeInfo tradeInfo = {};
    try {
      const auto &statusField = order.get<std::string>("status");
      if (statusField == "OPEN") {
        status = ORDER_STATUS_SUBMITTED;
      } else if (statusField == "CANCELLED") {
        status = ORDER_STATUS_CANCELLED;
      } else if (statusField == "EXECUTED") {
        status = ORDER_STATUS_FILLED;
        const auto &trade = order.get_child("trades");
        commission = trade.get<Volume>("commission");
        tradeInfo.price = trade.get<Price>("avg_price");
      } else {
        GetLog().Error(
            "Unknown order status received from trading system: \"%1%\".",
            statusField);
        continue;
      }
      remainingQuantity = order.get<Price>("remaining_quantity");
    } catch (const ptr::ptree_error &ex) {
      std::ostringstream error;
      error << "Field to read order status: \"" << ex.what() << "\"";
      throw Exception(error.str().c_str());
    }
    if (status == ORDER_STATUS_FILLED) {
      OnOrderStatusUpdate(GetContext().GetCurrentTime(), orderId, status,
                          remainingQuantity, commission, std::move(tradeInfo));
    } else {
      OnOrderStatusUpdate(GetContext().GetCurrentTime(), orderId, status,
                          remainingQuantity);
    }
  }
}

boost::optional<LivecoinTradingSystem::OrderCheckError>
LivecoinTradingSystem::CheckOrder(const trdk::Security &security,
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

  if (security.GetSymbol().GetCurrency() == CURRENCY_BTC) {
    if (vol < minBtcVolume) {
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
    throw TradingSystem::Error("Symbol is not supported by exchange");
  }

  boost::format requestParams("currencyPair=%1%&price=%2$.8f&quantity=%3$.8f");
  requestParams % product->second.requestId  // 1
      % RoundByPrecision(*price,
                         product->second.pricePrecisionPower)  // 2
      % qty;                                                   // 3

  TradingRequest request(
      side == ORDER_SIDE_BUY ? "/exchange/buylimit" : "/exchange/selllimit",
      m_settings, requestParams.str());
  const auto response =
      boost::get<1>(request.Send(*m_tradingSession, GetContext()));
  GetTradingLog().Write("debug-dump-order-submit\t%1%",
                        [&response](TradingRecord &record) {
                          record % ConvertToString(response, false);
                        });
  try {
    if (!response.get<bool>("added")) {
      throw Exception(
          ("Failed to add new order: " + ConvertToString(response, false))
              .c_str());
    }
    return boost::make_unique<OrderTransactionContext>(
        *this,
        product->second.requestId + "_" + response.get<std::string>("orderId"));
  } catch (const ptr::ptree_error &ex) {
    std::ostringstream error;
    error << "Field to read server response for new order request \""
          << ex.what() << "\"";
    throw Exception(error.str().c_str());
  }
}

void LivecoinTradingSystem::SendCancelOrderTransaction(const OrderId &orderId) {
  boost::format requestParams("currencyPair=%1%&orderId=%2%");
  {
    std::vector<std::string> params;
    params.reserve(2);
    boost::split(params, orderId.GetValue(), boost::is_any_of("_"));
    if (params.size() != 2) {
      throw Exception("Order ID has unknown format");
    }
    requestParams % params.front()  // 1
        % params.back();            // 2
  }

  const auto response = boost::get<1>(
      TradingRequest("/exchange/cancellimit", m_settings, requestParams.str())
          .Send(*m_tradingSession, GetContext()));
  GetTradingLog().Write("debug-dump-order-cancel\t%1%",
                        [&response](TradingRecord &record) {
                          record % ConvertToString(response, false);
                        });

  try {
    if (!response.get<bool>("cancelled")) {
      boost::format error("Failed to cancel order: \"%1%\"");
      const auto &message = response.get<std::string>("message");
      error % message;
      boost::istarts_with(message, "Failed to cancel order: can't find order")
          ? throw OrderIsUnknown(error.str().c_str())
          : throw Exception(error.str().c_str());
    }
  } catch (const ptr::ptree_error &ex) {
    std::ostringstream error;
    error << "Field to read server response for order cancel request \""
          << ex.what() << "\"";
    throw Exception(error.str().c_str());
  }
}

void LivecoinTradingSystem::OnTransactionSent(const OrderId &orderId) {
  Base::OnTransactionSent(orderId);
  m_pullingTask.AccelerateNextPulling();
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
