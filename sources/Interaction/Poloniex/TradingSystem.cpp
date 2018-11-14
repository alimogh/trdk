//
//    Created: 2018/11/14 15:26
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "TradingSystem.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Interaction;
using namespace Rest;
using namespace Poloniex;
namespace ptr = boost::property_tree;
namespace p = Poloniex;

boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
    const TradingMode &mode,
    Context &context,
    std::string instanceName,
    std::string title,
    const ptr::ptree &config) {
#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  const auto &result = boost::make_shared<p::TradingSystem>(
      App::GetInstance(), mode, context, std::move(instanceName),
      std::move(title), config);
  return result;
}

p::TradingSystem::TradingSystem(const App &,
                                const TradingMode &mode,
                                Context &context,
                                std::string instanceName,
                                std::string title,
                                const ptr::ptree &)
    : Base(mode, context, std::move(instanceName), std::move(title)),
      m_balances(*this, GetLog(), GetTradingLog()) {}

bool p::TradingSystem::IsConnected() const { return true; }

Volume p::TradingSystem::CalcCommission(const Qty &qty,
                                        const Price &price,
                                        const OrderSide &,
                                        const trdk::Security &) const {
  return (qty * price) * (0.2 / 100);
}

void p::TradingSystem::CreateConnection() {
  GetLog().Debug("Creating connection...");
}

std::unique_ptr<OrderTransactionContext> p::TradingSystem::SendOrderTransaction(
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

  throw MethodIsNotImplementedException("Not supported");
}

void p::TradingSystem::SendCancelOrderTransaction(
    const OrderTransactionContext &) {
  throw MethodIsNotImplementedException("Not supported");
}

Balances &p::TradingSystem::GetBalancesStorage() { return m_balances; }

boost::optional<OrderCheckError> p::TradingSystem::CheckOrder(
    const trdk::Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderSide &side) const {
  auto result = Base::CheckOrder(security, currency, qty, price, side);
  throw MethodIsNotImplementedException("Not supported");
}

bool p::TradingSystem::CheckSymbol(const std::string &symbol) const {
  return Base::CheckSymbol(symbol) && m_products.count(symbol) > 0;
}
