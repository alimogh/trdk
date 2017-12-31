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

CryptopiaTradingSystem::Settings::Settings(const IniSectionRef &conf,
                                           ModuleEventsLog &log)
    : Rest::Settings(conf, log),
      apiKey(conf.ReadKey("api_key")),
      apiSecret(Base64::Decode(conf.ReadKey("api_secret"))),
      nonces(apiKey, "Cryptopia", conf, log) {
  log.Info("API key: \"%1%\". API secret: %2%.",
           apiKey,                                     // 1
           apiSecret.empty() ? "not set" : "is set");  // 2
}

////////////////////////////////////////////////////////////////////////////////

CryptopiaTradingSystem::PrivateRequest::PrivateRequest(
    const std::string &name,
    NonceStorage &nonces,
    const Settings &settings,
    const std::string &params)
    : Base(name,
           "",
           Poco::Net::HTTPRequest::HTTP_POST,
           "application/json; charset=utf-8"),
      m_settings(settings),
      m_paramsDigest(Base64::Encode(CalcMd5Digest(params), false)),
      m_nonces(nonces) {
  SetBody(params);
}

CryptopiaTradingSystem::PrivateRequest::Response
CryptopiaTradingSystem::PrivateRequest::Send(net::HTTPClientSession &session,
                                             const Context &context) {
  Assert(!m_nonce);
  m_nonce.emplace(m_nonces.TakeNonce());

  const struct NonceScope {
    boost::optional<NonceStorage::TakenValue> &nonce;

    ~NonceScope() {
      Assert(nonce);
      nonce = boost::none;
    }
  } nonceScope = {m_nonce};

  const auto result = Base::Send(session, context);

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

 public:
  explicit OrderTransactionRequest(const std::string &name,
                                   NonceStorage &nonces,
                                   const Settings &settings,
                                   const std::string &params)
      : Base(name, nonces, settings, params) {}
  virtual ~OrderTransactionRequest() override = default;

 protected:
  virtual bool IsPriority() const override { return true; }
};

////////////////////////////////////////////////////////////////////////////////

CryptopiaTradingSystem::CryptopiaTradingSystem(const App &,
                                               const TradingMode &mode,
                                               Context &context,
                                               const std::string &instanceName,
                                               const IniSectionRef &conf)
    : Base(mode, context, instanceName),
      m_settings(conf, GetLog()),
      m_nonces(m_settings.nonces, GetLog()),
      m_balances(GetLog(), GetTradingLog()),
      m_balancesRequest(m_nonces, m_settings),
      m_openOrdersRequestsVersion(0),
      m_tradingSession(CreateSession("www.cryptopia.co.nz", m_settings, true)),
      m_pullingSession(CreateSession("www.cryptopia.co.nz", m_settings, false)),
      m_pullingTask(m_settings.pullingSetttings, GetLog()) {}

void CryptopiaTradingSystem::CreateConnection(const IniSectionRef &) {
  Assert(m_products.empty());

  try {
    UpdateBalances();
    m_products =
        RequestCryptopiaProductList(*m_tradingSession, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  m_pullingTask.AddTask(
      "Balances", 1,
      [this]() {
        UpdateBalances();
        return true;
      },
      m_settings.pullingSetttings.GetBalancesRequestFrequency());

  m_pullingTask.AccelerateNextPulling();
}

Volume CryptopiaTradingSystem::CalcCommission(
    const Volume &volume, const trdk::Security &security) const {
  const auto &productIndex = m_products.get<BySymbol>();
  const auto &productIt = productIndex.find(security.GetSymbol().GetSymbol());
  if (productIt == productIndex.cend()) {
    return 0;
  }
  return volume * productIt->feeRatio;
}

boost::optional<CryptopiaTradingSystem::OrderCheckError>
CryptopiaTradingSystem::CheckOrder(const trdk::Security &security,
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
    GetLog().Warn("Failed find product for \"%1%\" to check order.", security);
    return boost::none;
  }

  const CryptopiaProduct &product = *productIt;
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
      throw TradingSystem::Error("Order time-in-force type is not supported");
  }
  if (currency != security.GetSymbol().GetCurrency()) {
    throw TradingSystem::Error(
        "Trading system supports only security quote currency");
  }
  if (!price) {
    throw TradingSystem::Error("Market order is not supported");
  }

  const auto &productIndex = m_products.get<BySymbol>();
  const auto &product = productIndex.find(security.GetSymbol().GetSymbol());
  if (product == productIndex.cend()) {
    throw TradingSystem::Error("Symbol is not supported by exchange");
  }

  const auto &productId = product->id;

  class NewOrderRequest : public OrderTransactionRequest {
   public:
    explicit NewOrderRequest(const CryptopiaProductId &productId,
                             const std::string &side,
                             const Qty &qty,
                             const Price &price,
                             NonceStorage &nonces,
                             const Settings &settings)
        : OrderTransactionRequest("SubmitTrade",
                                  nonces,
                                  settings,
                                  CreateParams(productId, side, qty, price)) {}

   private:
    static std::string CreateParams(const CryptopiaProductId &productId,
                                    const std::string &side,
                                    const Qty &qty,
                                    const Price &price) {
      boost::format result(
          "{\"TradePairId\":%1%,\"Type\":\"%2%\",\"Rate\":%3$.8f,\"Amount\":%4$"
          ".8f}");
      result % productId  // 1
          % side          // 2
          % price         // 3
          % qty;          // 4
      return result.str();
    }
  } request(productId,
            product->NormalizeSide(side) == ORDER_SIDE_BUY ? "Buy" : "Sell",
            product->NormalizeQty(*price, qty, security),
            product->NormalizePrice(*price, security), m_nonces, m_settings);

  const auto response =
      boost::get<1>(request.Send(*m_tradingSession, GetContext()));
#ifdef DEV_VER
  GetTradingLog().Write("debug-dump-order-status\t%1%",
                        [&response](TradingRecord &record) {
                          record % ConvertToString(response, false);
                        });
#endif

  auto orderId = response.get<OrderId>("OrderId");
  if (orderId.GetValue() == "null") {
    const auto now = GetContext().GetCurrentTime();
    static size_t virtualOrderId = 1;
    orderId = "virtual_" + boost::lexical_cast<std::string>(virtualOrderId++);
    GetContext().GetTimer().Schedule(
        [this, orderId, now]() {
          try {
            OnOrderStatusUpdate(now, orderId, ORDER_STATUS_FILLED, 0);
          } catch (const OrderIsUnknown &) {
          }
        },
        m_timerScope);
  } else {
    SubscribeToOrderUpdates(product);
  }

  return boost::make_unique<OrderTransactionContext>(*this, std::move(orderId));
}

void CryptopiaTradingSystem::SendCancelOrderTransaction(
    const OrderId &orderId) {
  OrderTransactionRequest request(
      "CancelTrade", m_nonces, m_settings,
      "{\"OrderId\":" + boost::lexical_cast<std::string>(orderId) + "}");

  CancelOrderLock cancelOrderLock(m_cancelOrderMutex);
  const auto &response = request.Send(*m_tradingSession, GetContext());
  Verify(m_cancelingOrders.emplace(orderId).second);
  cancelOrderLock.unlock();

  UseUnused(response);
  const auto now = GetContext().GetCurrentTime();

  GetContext().GetTimer().Schedule(
      [this, orderId, now]() {
        const CancelOrderLock cancelOrderLock(m_cancelOrderMutex);
        AssertEq(1, m_cancelingOrders.count(orderId));
        m_cancelingOrders.erase(orderId);
        try {
          OnOrderCancel(now, orderId);
        } catch (const OrderIsUnknown &ex) {
          UseUnused(ex);
#ifdef DEV_VER
          GetLog().Debug("Failed to cancel order by scheduled task: \"%1%\".",
                         ex);
#endif
        }
      },
      m_timerScope);

#ifdef DEV_VER
  GetTradingLog().Write(
      "debug-dump-order-cancel\t%1%", [&response](TradingRecord &record) {
        record % ConvertToString(boost::get<1>(response), false);
      });
#endif
}

void CryptopiaTradingSystem::OnTransactionSent(const OrderId &orderId) {
  Base::OnTransactionSent(orderId);
  m_pullingTask.AccelerateNextPulling();
}

void CryptopiaTradingSystem::UpdateBalances() {
  const auto response = m_balancesRequest.Send(*m_pullingSession, GetContext());
  for (const auto &node : boost::get<1>(response)) {
    const auto &balance = node.second;
    m_balances.SetAvailableToTrade(balance.get<std::string>("Symbol"),
                                   balance.get<Volume>("Available"));
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

  boost::unordered_set<OrderId> orders;

  std::vector<CryptopiaProductList::iterator> emptyRequests;
  for (auto &request : openOrdersRequests) {
    const auto response =
        boost::get<1>(request.second->Send(*m_pullingSession, GetContext()));
    if (!response.empty()) {
      for (const auto &node : response) {
        Verify(orders.emplace(UpdateOrder(*request.first, node.second)).second);
      }
    } else {
      emptyRequests.emplace_back(request.first);
    }
  }

  for (const auto &activeOrder : GetActiveOrderList()) {
    if (orders.count(activeOrder) == 0) {
      {
        const CancelOrderLock cancelOrderLock(m_cancelOrderMutex);
        if (m_cancelingOrders.count(activeOrder)) {
          continue;
        }
      }
      try {
        OnOrderStatusUpdate(GetContext().GetCurrentTime(), activeOrder,
                            ORDER_STATUS_FILLED, 0);
      } catch (const OrderIsUnknown &) {
      }
    }
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

OrderId CryptopiaTradingSystem::UpdateOrder(
    const CryptopiaProduct & /*product*/, const ptr::ptree &node) {
#ifdef DEV_VER
  GetTradingLog().Write(
      "debug-dump-order-status\t%1%",
      [&](TradingRecord &record) { record % ConvertToString(node, false); });
#endif

  OrderId id;
  Qty remainingQty;
  pt::ptime time;

  try {
    id = node.get<OrderId>("OrderId");
    remainingQty = node.get<Price>("Remaining");
    {
      const auto &timeField = node.get<std::string>("TimeStamp");
      time = {gr::from_string(timeField.substr(0, 10)),
              pt::duration_from_string(timeField.substr(11, 8))};
    }
  } catch (const std::exception &ex) {
    boost::format error("Field to parse order : \"%1%\". Message: \"%2%\"");
    error % ex.what()                    // 1
        % ConvertToString(node, false);  // 2
    throw Exception(error.str().c_str());
  }

  // remainingQty = product.NormalizeQty(, remainingQty, );

  try {
    OnOrderStatusUpdate(time, id, ORDER_STATUS_SUBMITTED, remainingQty);
  } catch (const OrderIsUnknown &) {
  }

  return id;
}

void CryptopiaTradingSystem::SubscribeToOrderUpdates(
    const CryptopiaProductList::const_iterator &product) {
  class OpenOrdersRequest : public AccountRequest {
   public:
    explicit OpenOrdersRequest(const CryptopiaProductId &product,
                               NonceStorage &nonces,
                               const Settings &settings)
        : AccountRequest(
              "GetOpenOrders",
              nonces,
              settings,
              (boost::format("{\"TradePairId\": %1%, \"Count\": 1000}") %
               product)
                  .str()) {}
  };

  {
    const OrdersRequestsWriteLock lock(m_openOrdersRequestMutex);
    if (!m_openOrdersRequests
             .emplace(product, boost::make_shared<OpenOrdersRequest>(
                                   product->id, m_nonces, m_settings))
             .second) {
      return;
    }
    ++m_openOrdersRequestsVersion;
  }
  m_pullingTask.ReplaceTask(
      "Orders", 0, [this]() { return UpdateOrders(); },
      m_settings.pullingSetttings.GetActualOrdersRequestFrequency());
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateCryptopiaTradingSystem(
    const TradingMode &mode,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<CryptopiaTradingSystem>(
      App::GetInstance(), mode, context, instanceName, configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
