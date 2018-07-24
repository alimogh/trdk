//
//    Created: 2018/07/27 10:52 PM
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
using namespace Huobi;
using namespace Lib;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;
namespace net = Poco::Net;

boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &conf) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  const auto &result = boost::make_shared<Huobi::TradingSystem>(
      App::GetInstance(), mode, context, std::move(instanceName),
      std::move(title), conf);
  return result;
}

Huobi::TradingSystem::TradingSystem(const App &,
                                    const TradingMode &mode,
                                    Context &context,
                                    std::string instanceName,
                                    std::string title,
                                    const ptr::ptree &conf)
    : Base(mode, context, std::move(instanceName), std::move(title)),
      m_settings(conf, GetLog()),
      m_serverTimeDiff(
          GetUtcTimeZoneDiff(GetContext().GetSettings().GetTimeZone())),
      m_balancesRequest(
          "v1/account/accounts/" + m_settings.account + "/balance",
          net::HTTPRequest::HTTP_GET,
          "",
          GetContext(),
          m_settings,
          false,
          GetLog()),
      m_balances(*this, GetLog(), GetTradingLog()),
      m_tradingSession(CreateSession(m_settings, true)),
      m_pollingSession(CreateSession(m_settings, false)),
      m_pollingTask(m_settings.pollingSetttings, GetLog()) {}

bool Huobi::TradingSystem::IsConnected() const { return !m_products.empty(); }

void Huobi::TradingSystem::CreateConnection() {
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

Volume Huobi::TradingSystem::CalcCommission(const Qty &qty,
                                            const Price &price,
                                            const OrderSide &,
                                            const trdk::Security &) const {
  return (qty * price) * (0.2 / 100);
}

Balances &Huobi::TradingSystem::GetBalancesStorage() { return m_balances; }

void Huobi::TradingSystem::UpdateBalances() {
  boost::unordered_map<
      std::string, std::pair<boost::optional<Volume>, boost::optional<Volume>>>
      balances;
  const auto response = boost::get<1>(m_balancesRequest.Send(m_pollingSession));
  for (const auto &node : response.get_child("data.list")) {
    const auto &currency = node.second;
    auto balance = currency.get<Volume>("balance");
    const auto &type = currency.get<std::string>("type");
    std::pair<boost::optional<Volume>, boost::optional<Volume>> value;
    if (type == "trade") {
      value.first.emplace(std::move(balance));
    } else if (type == "frozen") {
      value.second.emplace(std::move(balance));
    } else {
      AssertEq(std::string("frozen"), type);
      continue;
    }
    const auto it =
        balances.emplace(currency.get<std::string>("currency"), value);
    if (it.second) {
      continue;
    }
    if (value.first) {
      Assert(!value.second);
      it.first->second.first = value.first;
    } else {
      Assert(value.second);
      it.first->second.second = value.second;
    }
  }
  for (const auto &balance : balances) {
    m_balances.Set(boost::to_upper_copy(balance.first),
                   balance.second.first.get_value_or(0),
                   balance.second.second.get_value_or(0));
  }
}

boost::optional<OrderCheckError> Huobi::TradingSystem::CheckOrder(
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

  if (!product->second.orderRequirements) {
    return boost::none;
  }

  if (qty < product->second.orderRequirements->minQty) {
    return OrderCheckError{product->second.orderRequirements->minQty};
  }
  if (qty > product->second.orderRequirements->maxQty) {
    return OrderCheckError{product->second.orderRequirements->maxQty};
  }

  return boost::none;
}

bool Huobi::TradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products.count(symbol) > 0;
}

std::unique_ptr<OrderTransactionContext>
Huobi::TradingSystem::SendOrderTransaction(trdk::Security &security,
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

  std::ostringstream requestParams;
  requestParams <<
      R"({"account-id":")" << m_settings.account << R"(","amount":")"
                << std::fixed << std::setprecision(product.qtyPrecision)
                << qty.Get() << R"(","price":")" << std::fixed
                << std::setprecision(product.pricePrecision) << price->Get()
                << R"(","symbol":")" << product.id << R"(","type":")"
                << (side == ORDER_SIDE_BUY ? "buy-limit" : "sell-limit")
                << R"("})";

  TradingRequest request("v1/order/orders/place", requestParams.str(),
                         GetContext(), m_settings, GetLog(), GetTradingLog());
  const auto response = boost::get<1>(request.Send(m_tradingSession));

  SubscribeToOrderUpdates();

  try {
    return boost::make_unique<OrderTransactionContext>(
        *this, response.get<OrderId>("data"));
  } catch (const ptr::ptree_error &ex) {
    boost::format error(
        R"(Wrong server response to the request "%1%" (%2%): "%3%")");
    error % request.GetName()            // 1
        % request.GetRequest().getURI()  // 2
        % ex.what();                     // 3
    throw Exception(error.str().c_str());
  }
}

void Huobi::TradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &transaction) {
  TradingRequest(
      "v1/order/orders/" +
          boost::lexical_cast<std::string>(transaction.GetOrderId()) +
          "/submitcancel",
      "", GetContext(), m_settings, GetLog(), GetTradingLog())
      .Send(m_tradingSession);
}

void Huobi::TradingSystem::OnTransactionSent(
    const OrderTransactionContext &transaction) {
  Base::OnTransactionSent(transaction);
  m_pollingTask.AccelerateNextPolling();
}

void Huobi::TradingSystem::SubscribeToOrderUpdates() {
  m_pollingTask.AddTask(
      "Orders", 0, [this]() { return UpdateOrders(); },
      m_settings.pollingSetttings.GetActualOrdersRequestFrequency(), true);
}

bool Huobi::TradingSystem::UpdateOrders() {
  const auto orders = GetActiveOrderContextList();
  if (orders.empty()) {
    return false;
  }
  for (const auto &order : orders) {
    UpdateOrder(order->GetOrderId());
  }
  return true;
}

void Huobi::TradingSystem::UpdateOrder(const OrderId &orderId) {
  try {
    const auto response =
        PrivateRequest(
            "v1/order/orders/" + boost::lexical_cast<std::string>(orderId),
            net::HTTPRequest::HTTP_GET, {}, GetContext(), m_settings, false,
            GetLog(), &GetTradingLog())
            .Send(m_pollingSession);
    const auto order = boost::get<1>(response).get_child("data");

    const auto &qty = order.get<Qty>("amount");
    const auto &filledQty = order.get<Qty>("field-amount");
    AssertGe(qty, filledQty);
    const auto &remainingQty = qty - filledQty;

    const Volume fee = 0;  // order.get<Volume>("field-fees");

    const auto &state = order.get<std::string>("state");
    if (state == "pre-submitted" || state == "submitting" ||
        state == "submitted") {
      OnOrderOpened(ConvertToPTimeFromMicroseconds(
                        order.get<int64_t>("created-at") * 1000) -
                        m_serverTimeDiff,
                    orderId);
    } else if (state == "filled") {
      AssertEq(0, remainingQty);
      OnOrderFilled(ConvertToPTimeFromMicroseconds(
                        order.get<int64_t>("finished-at") * 1000) -
                        m_serverTimeDiff,
                    orderId, fee);
    } else if (state == "canceled") {
      OnOrderCanceled(ConvertToPTimeFromMicroseconds(
                          order.get<int64_t>("canceled-at") * 1000) -
                          m_serverTimeDiff,
                      orderId, remainingQty, fee);
    } else {
      const auto time = order.get_optional<int64_t>("finished-at");
      OnOrderError(
          time ? ConvertToPTimeFromMicroseconds(*time * 1000) - m_serverTimeDiff
               : boost::get<0>(response),
          orderId, remainingQty, fee, "Unknown order status");
    }
  } catch (const std::exception &ex) {
    OnOrderError(pt::microsec_clock::local_time(), orderId, boost::none,
                 boost::none, ex.what());
  }
}
