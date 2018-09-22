/*******************************************************************************
 *   Created: 2017/12/07 23:17:33
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "CryptopiaTradingSystem.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::Crypto;
using namespace trdk::Interaction::Rest;

namespace net = Poco::Net;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

////////////////////////////////////////////////////////////////////////////////

namespace {

const auto newOrderSeachPeriod = pt::minutes(2);

OrderId ParseOrderId(const ptr::ptree &source) {
  return source.get<OrderId>("OrderId");
}

class CryptopiaOrderTransactionContext : public OrderTransactionContext {
 public:
  explicit CryptopiaOrderTransactionContext(
      CryptopiaTradingSystem &tradingSystem,
      OrderId &&orderId,
      bool isImmediatelyFilled)
      : OrderTransactionContext(tradingSystem, std::move(orderId)),
        m_isImmediatelyFilled(isImmediatelyFilled) {}

  bool IsImmediatelyFilled() const { return m_isImmediatelyFilled; }

 private:
  const bool m_isImmediatelyFilled;
};
}  // namespace

////////////////////////////////////////////////////////////////////////////////

CryptopiaTradingSystem::Settings::Settings(const ptr::ptree &conf,
                                           ModuleEventsLog &log)
    : Rest::Settings(conf, log),
      apiKey(conf.get<std::string>("config.auth.apiKey")),
      apiSecret(
          Base64::Decode(conf.get<std::string>("config.auth.apiSecret"))) {
  log.Info("API key: \"%1%\". API secret: %2%.",
           apiKey,                                     // 1
           apiSecret.empty() ? "not set" : "is set");  // 2
}

////////////////////////////////////////////////////////////////////////////////

CryptopiaTradingSystem::PrivateRequest::PrivateRequest(
    const std::string &name,
    NonceStorage &nonces,
    const Settings &settings,
    const std::string &params,
    bool isPriority,
    const Context &context,
    ModuleEventsLog &log,
    ModuleTradingLog *tradingLog)
    : Base(name,
           "",
           Poco::Net::HTTPRequest::HTTP_POST,
           "application/json; charset=utf-8",
           context,
           log,
           tradingLog),
      m_settings(settings),
      m_isPriority(isPriority),
      m_paramsDigest(Base64::Encode(CalcMd5Digest(params), false)),
      m_nonces(nonces) {
  SetBody(params);
}

CryptopiaTradingSystem::PrivateRequest::Response
CryptopiaTradingSystem::PrivateRequest::Send(
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

  return result;
}

void CryptopiaTradingSystem::PrivateRequest::PrepareRequest(
    const net::HTTPClientSession &session,
    const std::string &body,
    net::HTTPRequest &request) const {
  Assert(m_nonce);
  const auto &nonce = boost::lexical_cast<std::string>(m_nonce->Get());

  auto signatureSource = m_settings.apiKey + "POST";
  {
    std::string uri;
    Poco::URI::encode((session.secure() ? "https://" : "http://") +
                          session.getHost() + GetRequest().getURI(),
                      ":/?#=", uri);
    boost::to_lower(uri);
    signatureSource += uri;
  }
  signatureSource += nonce;
  signatureSource += m_paramsDigest;

  const auto &signature = Base64::Encode(
      Hmac::CalcSha256Digest(signatureSource, m_settings.apiSecret), false);

  request.set("Authorization",
              "amx " + m_settings.apiKey + ":" + signature + ":" + nonce);

  Base::PrepareRequest(session, body, request);
}

class CryptopiaTradingSystem::OrderTransactionRequest : public PrivateRequest {
 public:
  typedef PrivateRequest Base;

  explicit OrderTransactionRequest(const std::string &name,
                                   NonceStorage &nonces,
                                   const Settings &settings,
                                   const std::string &params,
                                   const Context &context,
                                   ModuleEventsLog &log,
                                   ModuleTradingLog &tradingLog)
      : Base(name, nonces, settings, params, true, context, log, &tradingLog) {}
  ~OrderTransactionRequest() override = default;

 protected:
  bool IsPriority() const override { return true; }
};

////////////////////////////////////////////////////////////////////////////////

CryptopiaTradingSystem::CryptopiaTradingSystem(const App &,
                                               const TradingMode &mode,
                                               Context &context,
                                               std::string instanceName,
                                               std::string title,
                                               const ptr::ptree &conf)
    : Base(mode, context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_nonces(boost::make_unique<NonceStorage::UnsignedInt64TimedGenerator>()),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_balancesRequest(m_nonces, m_settings, GetContext(), GetLog()),
      m_openOrdersRequestsVersion(0),
      m_tradingSession(CreateSession("www.cryptopia.co.nz", m_settings, true)),
      m_pollingSession(CreateSession("www.cryptopia.co.nz", m_settings, false)),
      m_pollingTask(m_settings.pollingSetttings, GetLog()) {}

void CryptopiaTradingSystem::CreateConnection() {
  Assert(m_products.empty());

  try {
    UpdateBalances();
    m_products =
        RequestCryptopiaProductList(m_tradingSession, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  m_pollingTask.AddTask(
      "Balances", 1,
      [this]() {
        UpdateBalances();
        return true;
      },
      m_settings.pollingSetttings.GetBalancesRequestFrequency(), true);

  m_pollingTask.AccelerateNextPolling();
}

Volume CryptopiaTradingSystem::CalcCommission(
    const Qty &qty,
    const Price &price,
    const OrderSide &,
    const trdk::Security &security) const {
  const auto &productIndex = m_products.get<BySymbol>();
  const auto &productIt = productIndex.find(security.GetSymbol().GetSymbol());
  if (productIt == productIndex.cend()) {
    return 0;
  }
  return (qty * price) * productIt->feeRatio;
}

boost::optional<OrderCheckError> CryptopiaTradingSystem::CheckOrder(
    const trdk::Security &security,
    const Currency &currency,
    const Qty &internalQty,
    const boost::optional<Price> &internalPrice,
    const OrderSide &side) const {
  {
    const auto result =
        Base::CheckOrder(security, currency, internalQty, internalPrice, side);
    if (result) {
      return result;
    }
  }

  Assert(internalPrice);
  if (!internalPrice) {
    //! Market price order is not supported.
    return boost::none;
  }

  const auto &productIndex = m_products.get<BySymbol>();
  const auto &productIt = productIndex.find(security.GetSymbol().GetSymbol());
  if (productIt == productIndex.cend()) {
    GetLog().Error(R"(Failed find product for "%1%" to check order.)",
                   security);
    throw Exception("Symbol is not supported by exchange");
  }

  const auto &product = *productIt;
  const auto &price = product.NormalizePrice(*internalPrice, security);

  if (price < product.minMaxPrice.first) {
    return OrderCheckError{
        boost::none,
        product.NormalizePrice(product.minMaxPrice.first, security)};
  } else if (product.minMaxPrice.second < price) {
    return OrderCheckError{
        boost::none,
        product.NormalizePrice(product.minMaxPrice.second, security)};
  }

  const auto &qty = product.NormalizeQty(*internalPrice, internalQty, security);

  {
    const auto volume = price * qty;
    if (volume < product.minMaxVolume.first) {
      return OrderCheckError{boost::none, boost::none,
                             product.minMaxVolume.first};
    } else if (product.minMaxVolume.second < volume) {
      return OrderCheckError{boost::none, boost::none,
                             product.minMaxVolume.second};
    }
  }

  if (qty < product.minMaxQty.first) {
    return OrderCheckError{
        product.NormalizeQty(price, product.minMaxQty.first, security)};
  } else if (product.minMaxQty.second < qty) {
    return OrderCheckError{
        product.NormalizeQty(price, product.minMaxQty.second, security)};
  }

  return boost::none;
}

bool CryptopiaTradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) &&
         m_products.get<BySymbol>().count(symbol) > 0;
}

std::unique_ptr<OrderTransactionContext>
CryptopiaTradingSystem::SendOrderTransaction(
    trdk::Security &security,
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

  const auto &productIndex = m_products.get<BySymbol>();
  const auto &product = productIndex.find(security.GetSymbol().GetSymbol());
  if (product == productIndex.cend()) {
    throw Exception("Symbol is not supported by exchange");
  }

  const auto &productId = product->id;
  const std::string actualSide =
      product->NormalizeSide(side) == ORDER_SIDE_BUY ? "Buy" : "Sell";
  const auto &actualQty = product->NormalizeQty(*price, qty, security);
  const auto &actualPrice = product->NormalizePrice(*price, security);

  class NewOrderRequest : public OrderTransactionRequest {
   public:
    explicit NewOrderRequest(const CryptopiaProductId &productId,
                             const std::string &side,
                             const Qty &qty,
                             const Price &price,
                             NonceStorage &nonces,
                             const Settings &settings,
                             const Context &context,
                             ModuleEventsLog &log,
                             ModuleTradingLog &tradingLog)
        : OrderTransactionRequest("SubmitTrade",
                                  nonces,
                                  settings,
                                  CreateParams(productId, side, qty, price),
                                  context,
                                  log,
                                  tradingLog) {}

   private:
    static std::string CreateParams(const CryptopiaProductId &productId,
                                    const std::string &side,
                                    const Qty &qty,
                                    const Price &price) {
      boost::format result(
          R"({"TradePairId":%1%,"Type":"%2%","Rate":%3%,"Amount":%4%})");
      result % productId  // 1
          % side          // 2
          % price         // 3
          % qty;          // 4
      return result.str();
    }
  } request(productId, actualSide, actualQty, actualPrice, m_nonces, m_settings,
            GetContext(), GetLog(), GetTradingLog());

  ptr::ptree response;
  const auto &startTime = GetContext().GetCurrentTime();
  try {
    response = boost::get<1>(request.Send(m_tradingSession));
  } catch (const Request::CommunicationErrorWithUndeterminedRemoteResult &ex) {
    GetLog().Debug(
        "Got error \"%1%\" at the new-order request sending. Trying to "
        "retrieve actual result...",
        ex.what());
    auto orderId = FindNewOrderId(productId, startTime, actualSide, actualQty,
                                  actualPrice);
    if (!orderId) {
      throw;
    }
    GetLog().Debug("Retrieved new order ID \"%1%\".", *orderId);
    return boost::make_unique<CryptopiaOrderTransactionContext>(
        *this, std::move(*orderId), false);
  }

  try {
    auto orderId = response.get<OrderId>("OrderId");
    const bool isImmediatelyFilled = orderId.GetValue() == "null";
    if (isImmediatelyFilled) {
      const auto &now = GetContext().GetCurrentTime();
      static size_t virtualOrderId = 1;
      orderId = "v" + boost::lexical_cast<std::string>(virtualOrderId++);
      m_pollingTask.AddTask(boost::lexical_cast<std::string>(orderId), 0,
                            [this, orderId, now]() {
                              try {
                                OnOrderFilled(now, orderId, boost::none);
                              } catch (const OrderIsUnknownException &) {
                                // Maybe already filled by periodic task.
                              }
                              return false;
                            },
                            0, true);
    } else {
      SubscribeToOrderUpdates(product);
      RegisterLastOrder(startTime, orderId);
    }

    return boost::make_unique<CryptopiaOrderTransactionContext>(
        *this, std::move(orderId), isImmediatelyFilled);

  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        R"(Wrong server response to the request "%1%" (%2%): "%3%")");
    error % request.GetName()            // 1
        % request.GetRequest().getURI()  // 2
        % ex.what();                     // 3
    throw Exception(error.str().c_str());
  }
}

void CryptopiaTradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &transaction) {
  if (boost::polymorphic_downcast<const CryptopiaOrderTransactionContext *>(
          &transaction)
          ->IsImmediatelyFilled()) {
    throw OrderIsUnknownException("Order was filled immediately at request");
  }

  OrderTransactionRequest request(
      "CancelTrade", m_nonces, m_settings,
      "{\"OrderId\":" +
          boost::lexical_cast<std::string>(transaction.GetOrderId()) + "}",
      GetContext(), GetLog(), GetTradingLog());

  //! todo Deadlock is here - SendCancelOrderTransaction locks order mutex and
  //!      OnOrderCancel will try to lock it too.
  const CancelOrderLock cancelOrderLock(m_cancelOrderMutex);
  request.Send(m_tradingSession);
  Verify(m_cancelingOrders.emplace(transaction.GetOrderId()).second);
}

void CryptopiaTradingSystem::OnTransactionSent(
    const OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pollingTask.AccelerateNextPolling();
}

void CryptopiaTradingSystem::UpdateBalances() {
  const auto response = m_balancesRequest.Send(m_pollingSession);
  for (const auto &node : boost::get<1>(response)) {
    const auto &balance = node.second;
    m_balances.Set(balance.get<std::string>("Symbol"),
                   balance.get<Volume>("Available"),
                   balance.get<Volume>("HeldForTrades"));
  }
}

bool CryptopiaTradingSystem::UpdateOrders() {
  boost::unordered_map<CryptopiaProductList::iterator,
                       boost::shared_ptr<Request>>
      openOrdersRequests;
  size_t version;
  {
    const OrdersRequestsReadLock lock(m_openOrdersRequestMutex);
    if (m_openOrdersRequests.empty()) {
      return false;
    }
    openOrdersRequests = m_openOrdersRequests;
    version = m_openOrdersRequestsVersion;
  }

  const auto activeOrders = GetActiveOrderContextList();
  boost::unordered_set<OrderId> actualOrders;

  std::vector<CryptopiaProductList::iterator> emptyRequests;
  for (auto &request : openOrdersRequests) {
    const auto response = boost::get<1>(request.second->Send(m_pollingSession));
    if (!response.empty()) {
      for (const auto &node : response) {
        const auto &order = node.second;
        try {
          const auto &it = actualOrders.emplace(ParseOrderId(order));
          Assert(it.second);
          OnOrderRemainingQtyUpdated(ParseTimeStamp(order), *it.first,
                                     order.get<Qty>("Remaining"));
        } catch (const std::exception &ex) {
          boost::format error(
              R"(Failed to read order: "%1%". Message: "%2%")");
          error % ex.what()                     // 1
              % ConvertToString(order, false);  // 2
          throw Exception(error.str().c_str());
        }
      }
    } else {
      emptyRequests.emplace_back(request.first);
    }
  }

  const auto &now = GetContext().GetCurrentTime();

  for (const auto &context : activeOrders) {
    const auto id = context->GetOrderId();
    if (actualOrders.count(id)) {
      continue;
    }
    {
      //! @todo Also see https://trello.com/c/Dmk5kjlA
      CancelOrderLock cancelOrderLock(m_cancelOrderMutex);
      const auto &canceledOrderIt = m_cancelingOrders.find(id);
      if (canceledOrderIt != m_cancelingOrders.cend()) {
        m_cancelingOrders.erase(canceledOrderIt);
        cancelOrderLock.unlock();
        // There are no other places which can remove the order from the active
        // list so this status update doesn't catch
        // OrderIsUnknownException-exception. No way to get remaining quantity.
        // Support ticked waits for answer:
        // https://www.cryptopia.co.nz/Support/SupportTicket?ticketId=135514
        OnOrderCanceled(now, id, boost::none, boost::none);
        continue;
      }
    }
    // There are no other places which can remove the order from the active
    // list so this status update doesn't catch
    // OrderIsUnknownException-exception.
    OnOrderFilled(now, id, boost::none);
  }

  {
    const OrdersRequestsWriteLock lock(m_openOrdersRequestMutex);
    if (version != m_openOrdersRequestsVersion) {
      return true;
    }
    for (const auto &request : emptyRequests) {
      m_openOrdersRequests.erase(request);
    }

    return !m_openOrdersRequests.empty();
  }
}

void CryptopiaTradingSystem::SubscribeToOrderUpdates(
    const CryptopiaProductList::const_iterator &product) {
  {
    const OrdersRequestsWriteLock lock(m_openOrdersRequestMutex);
    if (!m_openOrdersRequests
             .emplace(product, boost::make_shared<OpenOrdersRequest>(
                                   product->id, m_nonces, m_settings, false,
                                   GetContext(), GetLog(), GetTradingLog()))
             .second) {
      return;
    }
    ++m_openOrdersRequestsVersion;
  }
  m_pollingTask.ReplaceTask(
      "Orders", 0, [this]() { return UpdateOrders(); },
      m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
}

boost::optional<OrderId> CryptopiaTradingSystem::FindNewOrderId(
    const CryptopiaProductId &productId,
    const pt::ptime &startTime,
    const std::string &side,
    const Qty &qty,
    const Price &price) const {
  boost::optional<OrderId> result;
  OpenOrdersRequest request(productId, m_nonces, m_settings, true, GetContext(),
                            GetLog(), GetTradingLog());
  try {
    const auto response = boost::get<1>(request.Send(m_tradingSession));
    for (const auto &node : response) {
      const auto &order = node.second;
      const auto &id = ParseOrderId(order);
      if (IsIdRegisterInLastOrders(id) || GetActiveOrders().count(id) ||
          order.get<std::string>("Type") != side ||
          order.get<Qty>("Amount") != qty ||
          order.get<Price>("Rate") != price ||
          ParseTimeStamp(order) + newOrderSeachPeriod < startTime) {
        continue;
      }
      if (result) {
        GetLog().Warn(
            "Failed to find new order ID: Two or more orders are suitable "
            "(order ID: \"%1%\").",
            id);
        return boost::none;
      }
      result = id;
    }
  } catch (const CommunicationError &ex) {
    GetLog().Debug("Failed to find new order ID: \"%1%\".", ex.what());
    return boost::none;
  }

  return result;
}

pt::ptime CryptopiaTradingSystem::ParseTimeStamp(
    const ptr::ptree &source) const {
  const auto &field = source.get<std::string>("TimeStamp");
  pt::ptime result(gr::from_string(field.substr(0, 10)),
                   pt::duration_from_string(field.substr(11, 8)));
  result -= m_serverTimeDiff;
  return result;
}

void CryptopiaTradingSystem::RegisterLastOrder(const pt::ptime &startTime,
                                               const OrderId &id) {
  const auto &start = startTime - newOrderSeachPeriod;
  while (!m_lastOrders.empty() && m_lastOrders.front().first < start) {
    m_lastOrders.pop_front();
  }
  m_lastOrders.emplace_back(startTime, id);
}
bool CryptopiaTradingSystem::IsIdRegisterInLastOrders(const OrderId &id) const {
  for (const auto &order : m_lastOrders) {
    if (order.second == id) {
      return true;
    }
  }
  return false;
}

bool CryptopiaTradingSystem::AreWithdrawalSupported() const { return true; }

void CryptopiaTradingSystem::SendWithdrawalTransaction(
    const std::string &symbol,
    const Volume &volume,
    const std::string &address) {
  boost::format params(
      R"({"Currency":"%1%","Amount":"%2%","Address":"%3%"})");
  params % symbol  // 1
      % volume     // 2
      % address;   // 3
  PrivateRequest("SubmitWithdraw", m_nonces, m_settings, params.str(), false,
                 GetContext(), GetLog(), &GetTradingLog())
      .Send(m_tradingSession);
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateCryptopiaTradingSystem(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &configuration) {
  const auto &result = boost::make_shared<CryptopiaTradingSystem>(
      App::GetInstance(), mode, context, std::move(instanceName),
      std::move(title), configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
