/*******************************************************************************
 *   Created: 2018/01/18 01:27:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CexioTradingSystem.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Crypto;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace gr = boost::gregorian;

////////////////////////////////////////////////////////////////////////////////

CexioTradingSystem::Settings::Settings(const IniSectionRef &conf,
                                       ModuleEventsLog &log)
    : Rest::Settings(conf, log),
      username(conf.ReadKey("username")),
      apiKey(conf.ReadKey("api_key")),
      apiSecret(conf.ReadKey("api_secret")),
      nonces(apiKey, "Cexio", conf, log) {
  log.Info("Username: \"%1%\". API key: \"%2%\". API secret: %3%.",
           username,                                   // 1
           apiKey,                                     // 2
           apiSecret.empty() ? "not set" : "is set");  // 3
}

////////////////////////////////////////////////////////////////////////////////

CexioTradingSystem::PrivateRequest::PrivateRequest(
    const std::string &name,
    const std::string &params,
    const Settings &settings,
    NonceStorage::TakenValue &&nonce,
    bool isPriority,
    const Context &context,
    ModuleEventsLog &log,
    ModuleTradingLog *tradingLog)
    : Base(name, net::HTTPRequest::HTTP_POST, params, context, log, tradingLog),
      m_settings(settings),
      m_isPriority(isPriority),
      m_nonce(std::move(nonce)) {}

CexioTradingSystem::PrivateRequest::Response
CexioTradingSystem::PrivateRequest::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  const auto result = Base::Send(session);
  m_nonce.Use();
  return result;
}

void CexioTradingSystem::PrivateRequest::CreateBody(
    const net::HTTPClientSession &session, std::string &result) const {
  {
    using namespace trdk::Lib::Crypto;

    const auto &digest =
        Hmac::CalcSha256Digest(boost::lexical_cast<std::string>(m_nonce.Get()) +
                                   m_settings.username + m_settings.apiKey,
                               m_settings.apiSecret);
    boost::format auth("key=%1%&signature=%2%&nonce=%3%");
    auth % m_settings.apiKey                                            // 1
        % boost::to_upper_copy(EncodeToHex(&digest[0], digest.size()))  // 2
        % m_nonce.Get();                                                // 3
    AppendUriParams(auth.str(), result);
  }

  Base::CreateBody(session, result);
}

CexioTradingSystem::BalancesRequest::BalancesRequest(
    const Settings &settings,
    NonceStorage::TakenValue &&nonce,
    const Context &context,
    ModuleEventsLog &log)
    : PrivateRequest(
          "balance", "", settings, std::move(nonce), false, context, log) {}

CexioTradingSystem::OrderRequest::OrderRequest(const std::string &name,
                                               const std::string &params,
                                               const Settings &settings,
                                               NonceStorage::TakenValue &&nonce,
                                               const Context &context,
                                               ModuleEventsLog &log,
                                               ModuleTradingLog &tradingLog)
    : PrivateRequest(name,
                     params,
                     settings,
                     std::move(nonce),
                     true,
                     context,
                     log,
                     &tradingLog) {}

////////////////////////////////////////////////////////////////////////////////

CexioTradingSystem::CexioTradingSystem(const App &,
                                       const TradingMode &mode,
                                       Context &context,
                                       const std::string &instanceName,
                                       const IniSectionRef &conf)
    : Base(mode, context, instanceName),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_nonces(m_settings.nonces, GetLog()),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_tradingSession(CreateCexioSession(m_settings, true)),
      m_pollingSession(CreateCexioSession(m_settings, false)),
      m_pollingTask(m_settings.pollingSetttings, GetLog()) {}

void CexioTradingSystem::CreateConnection(const IniSectionRef &) {
  Assert(m_products.empty());

  try {
    UpdateBalances();
    m_products =
        RequestCexioProductList(m_tradingSession, GetContext(), GetLog());
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

Volume CexioTradingSystem::CalcCommission(
    const Qty &qty,
    const Price &price,
    const OrderSide &,
    const trdk::Security &security) const {
  return RoundByPrecision((qty * price) * (25 / 100),
                          security.GetPricePrecisionPower());
}

void CexioTradingSystem::UpdateBalances() {
  BalancesRequest request(m_settings, m_nonces.TakeNonce(), GetContext(),
                          GetLog());
  const auto response = boost::get<1>(request.Send(m_pollingSession));
  for (const auto &node : response) {
    if (node.first == "timestamp" || node.first == "username") {
      continue;
    }
    try {
      m_balances.Set(node.first, node.second.get<Volume>("available"),
                     node.second.get<Volume>("orders"));
    } catch (const std::exception &ex) {
      boost::format error("Failed to read balance: \"%1%\" (%2%)");
      error % ex.what()  // 1
          % ConvertToString(response, false);
      throw CommunicationError(error.str().c_str());
    }
  }
}

void CexioTradingSystem::UpdateOrders() {
  for (const auto &orderId : GetActiveOrderIdList()) {
    PrivateRequest request("get_order",
                           "id=" + boost::lexical_cast<std::string>(orderId),
                           m_settings, m_nonces.TakeNonce(), false,
                           GetContext(), GetLog(), &GetTradingLog());
    const auto response = boost::get<1>(request.Send(m_tradingSession));
    UpdateOrder(orderId, response);
  }
}

void CexioTradingSystem::UpdateOrder(const OrderId &id,
                                     const ptr::ptree &order) {
  AssertEq(id, order.get<OrderId>("orderId"));
  const auto &time = ParseTimeStamp("lastTxTime", order);
  const auto &field = order.get<std::string>("status");
  if (field == "a") {
    OnOrderOpened(time, id);
  } else if (field == "d") {
    // done(fully executed)
    OnOrderFilled(time, id, boost::none);
  } else if (field == "c" || field == "cd") {
    // canceled(not executed)
    // cancel - done(partially executed)
    OnOrderCanceled(time, id, order.get<Qty>("remains"), boost::none);
  } else {
    OnOrderError(time, id, boost::none, boost::none,
                 "Unknown order status received");
  }
}

boost::optional<CexioTradingSystem::OrderCheckError>
CexioTradingSystem::CheckOrder(const trdk::Security &security,
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

  const auto &productIt = m_products.find(security.GetSymbol().GetSymbol());
  if (productIt == m_products.cend()) {
    GetLog().Warn("Failed find product for \"%1%\" to check order.", security);
    return boost::none;
  }
  const auto &product = productIt->second;

  if (product.minSize && qty < *product.minSize) {
    return OrderCheckError{product.minSize};
  }

  if (product.maxSize && *product.maxSize < qty) {
    return OrderCheckError{product.maxSize};
  }

  if (price) {
    if (product.minPrice && *price < *product.minPrice) {
      return OrderCheckError{boost::none, product.minPrice};
    }
    if (product.maxPrice && *product.maxPrice < *price) {
      return OrderCheckError{boost::none, product.maxPrice};
    }
  }

  return boost::none;
}

bool CexioTradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products.count(symbol) > 0;
}

std::unique_ptr<OrderTransactionContext>
CexioTradingSystem::SendOrderTransaction(trdk::Security &security,
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

  const auto &productIt = m_products.find(security.GetSymbol().GetSymbol());
  if (productIt == m_products.cend()) {
    throw Exception("Symbol is not supported by exchange");
  }
  const auto &product = productIt->second;

  boost::format requestParams(product.requestParamsFormat);
  requestParams % (side == ORDER_SIDE_BUY ? "buy" : "sell")  // 1
      % qty                                                  // 2
      % *price;                                              // 3

  OrderRequest request("place_order/" + product.id, requestParams.str(),
                       m_settings, m_nonces.TakeNonce(), GetContext(), GetLog(),
                       GetTradingLog());

  const auto response = boost::get<1>(request.Send(m_tradingSession));

  try {
    return boost::make_unique<OrderTransactionContext>(
        *this, response.get<OrderId>("id"));
  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        "Wrong server response to the request \"%1%\" (%2%): \"%3%\"");
    error % request.GetName()            // 1
        % request.GetRequest().getURI()  // 2
        % ex.what();                     // 3
    throw Exception(error.str().c_str());
  }
}

void CexioTradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &transaction) {
  OrderRequest request(
      "cancel_order",
      "id=" + boost::lexical_cast<std::string>(transaction.GetOrderId()),
      m_settings, m_nonces.TakeNonce(), GetContext(), GetLog(),
      GetTradingLog());
  request.Send(m_tradingSession);
}

void CexioTradingSystem::OnTransactionSent(
    const OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pollingTask.AccelerateNextPolling();
}

pt::ptime CexioTradingSystem::ParseTimeStamp(const std::string &key,
                                             const ptr::ptree &source) const {
  pt::ptime result;
  auto field = source.get<std::string>(key);
  if (!field.empty() && field.back() == 'Z') {
    field.pop_back();
    result = pt::ptime(gr::from_string(field.substr(0, 10)),
                       pt::duration_from_string(field.substr(11)));
  } else {
    result = ConvertToPTimeFromMicroseconds(
        boost::lexical_cast<int64_t>(field) * 1000);
  }
  result -= m_serverTimeDiff;
  return result;
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateCexioTradingSystem(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<CexioTradingSystem>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
