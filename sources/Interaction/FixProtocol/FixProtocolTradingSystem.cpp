/*******************************************************************************
 *   Created: 2017/10/01 04:17:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "FixProtocolTradingSystem.hpp"
#include "Core/Security.hpp"
#include "FixProtocolIncomingMessages.hpp"
#include "FixProtocolOutgoingMessages.hpp"

using namespace trdk;
using namespace Lib;
using namespace trdk::Interaction::FixProtocol;

namespace fix = trdk::Interaction::FixProtocol;
namespace out = fix::Outgoing;

fix::TradingSystem::TradingSystem(const TradingMode &mode,
                                  size_t index,
                                  Context &context,
                                  const std::string &instanceName,
                                  const IniSectionRef &conf)
    : trdk::TradingSystem(mode, index, context, instanceName),
      Handler(context, conf, trdk::TradingSystem::GetLog()),
      m_client("Trade", *this),
      m_lastUsedOrderId(0) {}

Context &fix::TradingSystem::GetContext() {
  return trdk::TradingSystem::GetContext();
}
const Context &fix::TradingSystem::GetContext() const {
  return trdk::TradingSystem::GetContext();
}

ModuleEventsLog &fix::TradingSystem::GetLog() const {
  return trdk::TradingSystem::GetLog();
}

bool fix::TradingSystem::IsConnected() const { return m_client.IsConnected(); }

void fix::TradingSystem::CreateConnection(const IniSectionRef &) {
  GetLog().Debug("Connecting...");
  m_client.Connect();
  GetLog().Info("Connected.");
}

OrderId fix::TradingSystem::SendSellAtMarketPrice(trdk::Security &,
                                                  const Currency &,
                                                  const Qty &,
                                                  const OrderParams &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendSell(trdk::Security &security,
                                     const Currency &currency,
                                     const Qty &qty,
                                     const Price &price,
                                     const OrderParams &) {
  if (currency != security.GetSymbol().GetCurrency()) {
    throw Error("Trading system supports only security quote currency");
  }
  const auto orderId = ++m_lastUsedOrderId;
  m_client.Send(out::NewOrderSingle(orderId, security, ORDER_SIDE_SELL, qty,
                                    price, GetStandardOutgoingHeader()));
  return orderId;
}

OrderId fix::TradingSystem::SendSellImmediatelyOrCancel(trdk::Security &,
                                                        const Currency &,
                                                        const Qty &,
                                                        const Price &,
                                                        const OrderParams &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendSellAtMarketPriceImmediatelyOrCancel(
    trdk::Security &, const Currency &, const Qty &, const OrderParams &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendBuyAtMarketPrice(trdk::Security &,
                                                 const Currency &,
                                                 const Qty &,
                                                 const OrderParams &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendBuy(trdk::Security &security,
                                    const Currency &currency,
                                    const Qty &qty,
                                    const Price &price,
                                    const OrderParams &) {
  if (currency != security.GetSymbol().GetCurrency()) {
    throw Error("Trading system supports only security quote currency");
  }
  const auto orderId = ++m_lastUsedOrderId;
  m_client.Send(out::NewOrderSingle(orderId, security, ORDER_SIDE_BUY, qty,
                                    price, GetStandardOutgoingHeader()));
  return orderId;
}

OrderId fix::TradingSystem::SendBuyImmediatelyOrCancel(trdk::Security &,
                                                       const Currency &,
                                                       const Qty &,
                                                       const Price &,
                                                       const OrderParams &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
    trdk::Security &, const Currency &, const Qty &, const OrderParams &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

void fix::TradingSystem::SendCancelOrder(const OrderId &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

void fix::TradingSystem::OnConnectionRestored() {
  throw MethodDoesNotImplementedError(
      "fix::TradingSystem::OnConnectionRestored is not implemented");
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
    const TradingMode &mode,
    size_t index,
    Context &context,
    const std::string &instanceName,
    const IniSectionRef &configuration) {
  const auto &result = boost::make_shared<fix::TradingSystem>(
      mode, index, context, instanceName, configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
