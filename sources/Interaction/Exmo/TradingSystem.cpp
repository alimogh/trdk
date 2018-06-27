//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "TradingSystem.hpp"
#include "Session.hpp"
#include <boost/unordered/unordered_set.hpp>

using namespace trdk;
using namespace Interaction;
using namespace Rest;
using namespace Exmo;
using namespace Lib;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;

namespace trdk {
namespace Interaction {
namespace Exmo {
class OrderTransactionContext : public trdk::OrderTransactionContext {
 public:
  explicit OrderTransactionContext(TradingSystem &tradingSystem,
                                   OrderId &&orderId)
      : trdk::OrderTransactionContext(tradingSystem, std::move(orderId)) {}

  bool RegisterTrade(const std::string &id) {
    return m_trades.emplace(id).second;
  }

 private:
  boost::unordered_set<std::string> m_trades;
};
}  // namespace Exmo
}  // namespace Interaction
}  // namespace trdk

boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &conf) {
  const auto &result = boost::make_shared<Exmo::TradingSystem>(
      App::GetInstance(), mode, context, std::move(instanceName),
      std::move(title), conf);
  return result;
}

Exmo::TradingSystem::TradingSystem(const App &,
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
      m_balancesRequest(
          "user_info", "", GetContext(), m_settings, m_nonces, false, GetLog()),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_tradingSession(CreateSession(m_settings, true)),
      m_pollingSession(CreateSession(m_settings, false)),
      m_pollingTask(m_settings.pollingSetttings, GetLog()) {}

bool Exmo::TradingSystem::IsConnected() const { return !m_products.empty(); }

void Exmo::TradingSystem::CreateConnection() {
  Assert(m_products.empty());

  try {
    UpdateBalances();
    m_products = RequestProductList(m_tradingSession, GetContext(), GetLog());
  } catch (const std::exception &ex) {
    throw ConnectError(ex.what());
  }

  // m_pollingTask.AddTask(
  //   "Orders", 0,
  //   [this]() {
  //   UpdateOrders();
  //   return true;
  // },
  //   m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
  m_pollingTask.AddTask(
      "Balances", 1,
      [this]() {
        UpdateBalances();
        return true;
      },
      m_settings.pollingSetttings.GetBalancesRequestFrequency(), true);

  m_pollingTask.AccelerateNextPolling();
}

Volume Exmo::TradingSystem::CalcCommission(const Qty &qty,
                                           const Price &price,
                                           const OrderSide &,
                                           const trdk::Security &) const {
  return (qty * price) * (0.2 / 100);
}

Balances &Exmo::TradingSystem::GetBalancesStorage() { return m_balances; }

void Exmo::TradingSystem::UpdateBalances() {
  boost::unordered_map<std::string, std::pair<Volume, Volume>> buffer;
  const auto response = boost::get<1>(m_balancesRequest.Send(m_pollingSession));
  for (const auto &node : response.get_child("balances")) {
    Verify(buffer
               .emplace(node.first,
                        std::make_pair(node.second.get_value<Volume>(), 0))
               .second);
  }
  for (const auto &node : response.get_child("reserved")) {
    AssertEq(1, buffer.count(node.first));
    buffer[node.first].second = node.second.get_value<Volume>();
  }
  for (const auto &balance : buffer) {
    auto available = balance.second.first;
    auto locked = balance.second.second;
    m_balances.Set(balance.first, std::move(available), std::move(locked));
  }
}

boost::optional<OrderCheckError> Exmo::TradingSystem::CheckOrder(
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
  const auto &product = m_products.find(security.GetSymbol().GetSymbol());
  if (product == m_products.cend()) {
    GetLog().Error("Failed find product for \"%1%\" to check order.", security);
    throw Exception("Product is unknown");
  }

  if (qty < product->second.minQty) {
    return OrderCheckError{product->second.minQty};
  }
  if (qty > product->second.maxQty) {
    return OrderCheckError{product->second.maxQty};
  }

  if (price) {
    if (*price < product->second.minPrice) {
      return OrderCheckError{boost::none, product->second.minPrice};
    }
    if (*price > product->second.maxPrice) {
      return OrderCheckError{boost::none, product->second.maxPrice};
    }

    const auto volume = *price * qty;
    if (volume < product->second.minVolume) {
      return OrderCheckError{boost::none, product->second.minVolume};
    }
    if (volume > product->second.maxVolume) {
      return OrderCheckError{boost::none, product->second.maxVolume};
    }
  }

  return boost::none;
}

bool Exmo::TradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products.count(symbol) > 0;
}

std::unique_ptr<trdk::OrderTransactionContext>
Exmo::TradingSystem::SendOrderTransaction(trdk::Security &security,
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
  const auto &productIt = m_products.find(symbol);
  if (productIt == m_products.cend()) {
    throw Exception("Product is unknown");
  }
  const auto &product = productIt->second;

  boost::format requestParams("pair=%1%&quantity=%2%&price=%3%&type=%4%");
  requestParams % product.id                        // 1
      % qty                                         // 2
      % *price                                      // 3
      % (side == ORDER_SIDE_BUY ? "buy" : "sell");  // 4

  TradingRequest request("order_create", requestParams.str(), GetContext(),
                         m_settings, m_nonces, GetLog(), GetTradingLog());
  const auto response = boost::get<1>(request.Send(m_tradingSession));

  SubscribeToOrderUpdates();

  try {
    return boost::make_unique<Exmo::OrderTransactionContext>(
        *this, response.get<OrderId>("order_id"));
  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        R"(Wrong server response to the request "%1%" (%2%): "%3%")");
    error % request.GetName()            // 1
        % request.GetRequest().getURI()  // 2
        % ex.what();                     // 3
    throw Exception(error.str().c_str());
  }
}

void Exmo::TradingSystem::SendCancelOrderTransaction(
    const trdk::OrderTransactionContext &transaction) {
  TradingRequest request(
      "order_cancel",
      "order_id=" + boost::lexical_cast<std::string>(transaction.GetOrderId()),
      GetContext(), m_settings, m_nonces, GetLog(), GetTradingLog());
  const CancelOrderLock lock(m_cancelOrderMutex);
  request.Send(m_tradingSession);
  Verify(m_cancelingOrders.emplace(transaction.GetOrderId()).second);
}

void Exmo::TradingSystem::OnTransactionSent(
    const trdk::OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pollingTask.AccelerateNextPolling();
}

void Exmo::TradingSystem::SubscribeToOrderUpdates() {
  m_pollingTask.ReplaceTask(
      "Orders", 0, [this]() { return UpdateOrders(); },
      m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
}

bool Exmo::TradingSystem::UpdateOrders() {
  const auto activeOrders = GetActiveOrderContextList();
  if (activeOrders.empty()) {
    return false;
  }

  for (const auto &order : activeOrders) {
    try {
      UpdateOrder(order->GetOrderId());
    } catch (const Exception &ex) {
      GetLog().Error(R"(Failed to update order "%1%": "%2%".)",
                     order->GetOrderId(),  // 1
                     ex);                  // 2
    }
  }

  boost::unordered_set<OrderId> cancelingOrders;
  {
    const CancelOrderLock lock(m_cancelOrderMutex);
    cancelingOrders = m_cancelingOrders;
  }
  boost::unordered_set<OrderId> actualOrders;
  {
    const auto response = boost::get<1>(
        PrivateRequest("user_open_orders", "", GetContext(), m_settings,
                       m_nonces, false, GetLog(), &GetTradingLog())
            .Send(m_pollingSession));
    for (const auto &symbol : response) {
      for (const auto &order : symbol.second) {
        Verify(
            actualOrders.emplace(order.second.get<OrderId>("order_id")).second);
      }
    }
  }

  {
    const auto now = GetContext().GetCurrentTime();
    for (const auto &order : activeOrders) {
      const auto &orderId = order->GetOrderId();
      if (actualOrders.count(orderId)) {
        continue;
      }
      if (cancelingOrders.count(orderId)) {
        OnOrderCanceled(now, orderId, boost::none, boost::none);
        const CancelOrderLock lock(m_cancelOrderMutex);
        const auto &it = m_cancelingOrders.find(orderId);
        Assert(it != m_cancelingOrders.cend());
        m_cancelingOrders.erase(it);
      } else {
        OnOrderFilled(now, orderId, boost::none);
      }
    }
  }

  return true;
}

void Exmo::TradingSystem::UpdateOrder(const OrderId &orderId) {
  ptr::ptree response;
  try {
    response = boost::get<1>(
        PrivateRequest("order_trades",
                       "order_id=" + boost::lexical_cast<std::string>(orderId),
                       GetContext(), m_settings, m_nonces, false, GetLog(),
                       &GetTradingLog())
            .Send(m_pollingSession));
  } catch (const OrderIsUnknownException &) {
    return;
  }
  for (const auto &node : response.get_child("trades")) {
    const auto &tradeNode = node.second;
    const auto time =
        pt::from_time_t(tradeNode.get<time_t>("date")) - m_serverTimeDiff;
    const auto tradeId = tradeNode.get<std::string>("trade_id");
    Trade trade = {tradeNode.get<Price>("price"),
                   tradeNode.get<Qty>("quantity"), tradeId};
    OnTrade(time, orderId, std::move(trade),
            [&tradeId](trdk::OrderTransactionContext &orderContext) {
              return boost::polymorphic_cast<Exmo::OrderTransactionContext *>(
                         &orderContext)
                  ->RegisterTrade(tradeId);
            });
  }
}
