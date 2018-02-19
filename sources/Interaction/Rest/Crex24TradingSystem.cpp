/*******************************************************************************
 *   Created: 2018/02/16 04:32:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Crex24TradingSystem.hpp"
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

Crex24TradingSystem::Settings::Settings(const IniSectionRef &conf,
                                        ModuleEventsLog &log)
    : Rest::Settings(conf, log),
      apiKey(conf.ReadKey("api_key")),
      apiSecret(conf.ReadKey("api_secret")) {
  log.Info("API key: \"%1%\". API secret: %2%.",
           apiKey,                                     // 1
           apiSecret.empty() ? "not set" : "is set");  // 2
}

////////////////////////////////////////////////////////////////////////////////

Crex24TradingSystem::PrivateRequest::PrivateRequest(
    const std::string &name,
    const std::string &params,
    const Settings &settings,
    NonceStorage &nonces,
    bool isPriority,
    const Context &context,
    ModuleEventsLog &log,
    ModuleTradingLog *tradingLog)
    : Base("/CryptoExchangeService/BotTrade/",
           name,
           net::HTTPRequest::HTTP_POST,
           std::string(),
           context,
           log,
           tradingLog),
      m_settings(settings),
      m_isPriority(isPriority),
      m_nonces(nonces) {
  SetBody(params);
}

Crex24TradingSystem::PrivateRequest::Response
Crex24TradingSystem::PrivateRequest::Send(
    std::unique_ptr<net::HTTPSClientSession> &session) {
  Assert(!m_nonce);
  m_nonce.emplace(m_nonces.TakeNonce());

  const struct NonceScope {
    boost::optional<NonceStorage::TakenValue> &nonce;

    ~NonceScope() {
      Assert(nonce);
      nonce = boost::none;
    }
  } nonceScope = {m_nonce};

  const auto result = Base::Send(session);

  Assert(m_nonce);
  m_nonce->Use();

  {
    if (boost::get<1>(result).get_child_optional("Error")) {
      std::ostringstream message;
      message << "The server returned an error in response to the request \""
              << GetName() << "\" (" << GetRequest().getURI() << "): \""
              << boost::get<1>(result).get<std::string>("Error.Message")
              << "\"";
      throw Exception(message.str().c_str());
    }
  }

  return result;
}

void Crex24TradingSystem::PrivateRequest::PrepareRequest(
    const net::HTTPClientSession &session,
    const std::string &body,
    net::HTTPRequest &request) const {
  using namespace trdk::Lib::Crypto;
  request.set("Key", m_settings.apiKey);
  {
    const auto &digest = Hmac::CalcSha512Digest(body, m_settings.apiSecret);
    request.set("Sign", EncodeToHex(&digest[0], digest.size()));
  }
  {
    Assert(m_nonce);
    request.set("Nonce", boost::lexical_cast<std::string>(m_nonce->Get()));
  }
  Base::PrepareRequest(session, body, request);
}

bool Crex24TradingSystem::PrivateRequest::IsPriority() const {
  return m_isPriority;
}

Crex24TradingSystem::BalancesRequest::BalancesRequest(const Settings &settings,
                                                      NonceStorage &nonces,
                                                      const Context &context,
                                                      ModuleEventsLog &log)
    : PrivateRequest("ReturnBalances",
                     "{\"NeedNull\":\"false\"}",
                     settings,
                     nonces,
                     false,
                     context,
                     log) {}

////////////////////////////////////////////////////////////////////////////////

Crex24TradingSystem::Crex24TradingSystem(const App &,
                                         const TradingMode &mode,
                                         Context &context,
                                         const std::string &instanceName,
                                         const IniSectionRef &conf)
    : Base(mode, context, instanceName),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_nonces(boost::make_unique<NonceStorage::UnsignedInt64TimedGenerator>()),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_balancesRequest(m_settings, m_nonces, GetContext(), GetLog()),
      m_tradingSession(CreateCrex24Session(m_settings, true)),
      m_pollingSession(CreateCrex24Session(m_settings, false)),
      m_pollingTask(m_settings.pollingSetttings, GetLog()) {}

bool Crex24TradingSystem::IsConnected() const { return !m_products.empty(); }

void Crex24TradingSystem::CreateConnection(const IniSectionRef &) {
  Assert(m_products.empty());

  try {
    UpdateBalances();
    m_products =
        RequestCrex24ProductList(m_tradingSession, GetContext(), GetLog());
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

Volume Crex24TradingSystem::CalcCommission(const Qty &qty,
                                           const Price &price,
                                           const OrderSide &,
                                           const trdk::Security &) const {
  return (qty * price) * (0.1 / 100);
}

void Crex24TradingSystem::UpdateBalances() {
  const auto response = boost::get<1>(m_balancesRequest.Send(m_pollingSession));
  for (const auto &node : response) {
    const auto &currency = node.second;
    m_balances.Set(currency.get<std::string>("Name"),
                   currency.get<Volume>("AvailableBalances"),
                   currency.get<Volume>("InOrderBalances"));
  }
}

void Crex24TradingSystem::UpdateOrders() {}

boost::optional<Crex24TradingSystem::OrderCheckError>
Crex24TradingSystem::CheckOrder(const trdk::Security &security,
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
  return boost::none;
}

bool Crex24TradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products.count(symbol) > 0;
}

std::unique_ptr<OrderTransactionContext>
Crex24TradingSystem::SendOrderTransaction(trdk::Security &security,
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

  boost::format requestParams(
      "{\"Pair\":\"%1%\",\"Course\":%2%,\"Volume\":%3%}");
  requestParams % security.GetSymbol().GetSymbol()  // 1
      % *price                                      // 2
      % qty;                                        // 3

  PrivateRequest request(side == ORDER_SIDE_BUY ? "Buy" : "Sell",
                         requestParams.str(), m_settings, m_nonces, true,
                         GetContext(), GetLog(), &GetTradingLog());

  const auto response = boost::get<1>(request.Send(m_tradingSession));

  try {
    return boost::make_unique<OrderTransactionContext>(
        *this, response.get<OrderId>("Id"));
  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        "Wrong server response to the request \"%1%\" (%2%): \"%3%\"");
    error % request.GetName()            // 1
        % request.GetRequest().getURI()  // 2
        % ex.what();                     // 3
    throw Exception(error.str().c_str());
  }
}

void Crex24TradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &transaction) {
  const auto response = boost::get<1>(
      PrivateRequest(
          "CancelOrder",
          (boost::format("{\"OrderId\":%1%}") % transaction.GetOrderId()).str(),
          m_settings, m_nonces, true, GetContext(), GetLog(), &GetTradingLog())
          .Send(m_tradingSession));
}

void Crex24TradingSystem::OnTransactionSent(
    const OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pollingTask.AccelerateNextPolling();
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateCrex24TradingSystem(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<Crex24TradingSystem>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
