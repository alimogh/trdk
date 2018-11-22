//
//    Created: 2018/10/14 8:57 PM
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

using namespace trdk;
using namespace Interaction;
using namespace Rest;
using namespace Kraken;
using namespace Lib;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;

boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &conf) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  const auto &result = boost::make_shared<Kraken::TradingSystem>(
      App::GetInstance(), mode, context, std::move(instanceName),
      std::move(title), conf);
  return result;
}

Kraken::TradingSystem::TradingSystem(const App &,
                                     const TradingMode &mode,
                                     Context &context,
                                     std::string instanceName,
                                     std::string title,
                                     const ptr::ptree &conf)
    : Base(mode, context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_nonces(std::make_unique<NonceStorage::UnsignedInt64SecondsGenerator>()),
      m_balancesRequest(
          "Balance", "", GetContext(), m_settings, m_nonces, false, GetLog()),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_tradingSession(CreateSession(m_settings, true)),
      m_pollingSession(CreateSession(m_settings, false)),
      m_pollingTask(m_settings.pollingSetttings, GetLog()) {}

bool Kraken::TradingSystem::IsConnected() const { return !m_products.empty(); }

void Kraken::TradingSystem::CreateConnection() {
  Assert(m_products.empty());

  try {
    UpdateBalances();
    m_products = RequestProductList(m_tradingSession, GetContext(), GetLog());
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

Volume Kraken::TradingSystem::CalcCommission(
    const Qty &qty,
    const Price &price,
    const OrderSide &,
    const trdk::Security &security) const {
  const auto &product = m_products.find(security.GetSymbol().GetSymbol());
  if (product == m_products.cend()) {
    GetLog().Error(R"(Failed find product for "%1%" to calculate commission.)",
                   security);
    throw Exception("Symbol is not supported by exchange");
  }
  return (qty * price) * product->second.feeRatio;
}

Balances &Kraken::TradingSystem::GetBalancesStorage() { return m_balances; }

void Kraken::TradingSystem::UpdateBalances() {
  const auto response = boost::get<1>(m_balancesRequest.Send(m_pollingSession));
  for (const auto &node : response) {
    m_balances.Set(ResolveSymbol(node.first), node.second.get_value<Volume>(),
                   0);
  }
}

boost::optional<OrderCheckError> Kraken::TradingSystem::CheckOrder(
    const trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderSide &side) const {
  auto result = Base::CheckOrder(security, currency, qty, price, side);

  const auto &product = m_products.find(security.GetSymbol().GetSymbol());
  if (product == m_products.cend()) {
    GetLog().Error(R"(Failed find product for "%1%" to check order.)",
                   security);
    throw Exception("Symbol is not supported by exchange");
  }

  if (price) {
    if ((!result || !result->volume) &&
        (*price * qty < product->second.minVolume)) {
      if (!result) {
        result = OrderCheckError{};
      }
      result->volume = product->second.minVolume;
    }
  }

  return result;
}

bool Kraken::TradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products.count(symbol) > 0;
}

#pragma warning(push)
#pragma warning(disable : 4702)  // Warning	C4702	unreachable code
std::unique_ptr<OrderTransactionContext>
Kraken::TradingSystem::SendOrderTransaction(trdk::Security &security,
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

  boost::format requestParams(
      "pair=%1%&type=%2%&ordertype=limit&price=%3%&volume=%4%");
  requestParams % product.id                                        // 1
      % (side == ORDER_SIDE_BUY ? "buy" : "sell")                   // 2
      % RoundByPrecisionPower(*price, product.pricePrecisionPower)  // 3
      % qty;                                                        // 4

  PrivateRequest request("AddOrder", requestParams.str(), GetContext(),
                         m_settings, m_nonces, true, GetLog(),
                         &GetTradingLog());
  SubscribeToOrderUpdates();
  const auto response = boost::get<1>(request.Send(m_tradingSession));

  try {
    for (const auto &node : response.get_child("txid")) {
      return boost::make_unique<OrderTransactionContext>(
          *this, node.second.get_value<std::string>());
    }
  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        R"(Wrong server response to the request "%1%" (%2%): "%3%")");
    error % request.GetName()            // 1
        % request.GetRequest().getURI()  // 2
        % ex.what();                     // 3
    throw Exception(error.str().c_str());
  }
  boost::format error(R"(Wrong server response to the request "%1%" (%2%))");
  error % request.GetName()             // 1
      % request.GetRequest().getURI();  // 2
  throw Exception(error.str().c_str());
}
#pragma warning(pop)

void Kraken::TradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &transaction) {
  PrivateRequest("CancelOrder", "txid=" + transaction.GetOrderId().GetValue(),
                 GetContext(), m_settings, m_nonces, true, GetLog(),
                 &GetTradingLog())
      .Send(m_tradingSession);
}

void Kraken::TradingSystem::OnTransactionSent(
    const OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pollingTask.AccelerateNextPolling();
}

void Kraken::TradingSystem::SubscribeToOrderUpdates() {
  m_pollingTask.AddTask(
      "Orders", 0, [this]() { return UpdateOrders(); },
      m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
}

bool Kraken::TradingSystem::UpdateOrders() {
  const auto orders = GetActiveOrderContextList();
  if (orders.empty()) {
    return false;
  }
  std::string orderIdsRequest;
  for (const auto &order : orders) {
    if (!orderIdsRequest.empty()) {
      orderIdsRequest.push_back(',');
    }
    orderIdsRequest += order->GetOrderId().GetValue();
  }
  {
    const auto response =
        PrivateRequest("QueryOrders", "trades=false&txid=" + orderIdsRequest,
                       GetContext(), m_settings, m_nonces, false, GetLog(),
                       &GetTradingLog())
            .Send(m_pollingSession);
    for (const auto &node : boost::get<1>(response)) {
      const auto &time = boost::get<0>(response);
      const auto &id = node.first;
      try {
        UpdateOrder(time, id, node.second);
      } catch (const std::exception &ex) {
        OnOrderError(time, id, boost::none, boost::none, ex.what());
      }
    }
  }
  return true;
}

void Kraken::TradingSystem::UpdateOrder(const pt::ptime &time,
                                        const OrderId &id,
                                        const ptr::ptree &order) {
  const auto &status = order.get<std::string>("status");
  if (status == "pending" || status == "open") {
    OnOrderOpened(
        ConvertToPTimeFromMicroseconds(
            (static_cast<int64_t>(order.get<double>("opentm")) * 1000) * 1000) -
            m_serverTimeDiff,
        id);
    return;
  }

  const auto &fee = order.get<Volume>("fee");
  const auto expireTime =
      ConvertToPTimeFromMicroseconds(
          (static_cast<int64_t>(order.get<double>("closetm")) * 1000) * 1000) -
      m_serverTimeDiff;
  const auto &qty = order.get<Qty>("vol");
  const auto &filledQty = order.get<Qty>("vol_exec");
  AssertGe(qty, filledQty);
  const auto &remainingQty = qty - filledQty;

  if (status == "closed") {
    AssertEq(0, remainingQty);
    OnOrderFilled(expireTime, id, fee);
  } else if (status == "canceled") {
    OnOrderCanceled(expireTime, id, remainingQty, fee);
  } else if (status == "expired") {
    OnOrderRejected(expireTime, id, remainingQty, fee);
  } else {
    OnOrderError(time, id, remainingQty, fee, "Unknown order status");
  }
}
