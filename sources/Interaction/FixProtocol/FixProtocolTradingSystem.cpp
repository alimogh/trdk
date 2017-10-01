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
#include "FixProtocolIncomingMessages.hpp"
#include "FixProtocolOutgoingMessages.hpp"

using namespace trdk;
using namespace Lib;
using namespace Interaction::FixProtocol;

namespace fix = Interaction::FixProtocol;

fix::TradingSystem::TradingSystem(const TradingMode &mode,
                                  size_t index,
                                  Context &context,
                                  const std::string &instanceName,
                                  const IniSectionRef &conf)
    : trdk::TradingSystem(mode, index, context, instanceName),
      Handler(context, conf, trdk::TradingSystem::GetLog()),
      m_client("Trade", *this) {}

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

OrderId fix::TradingSystem::SendSellAtMarketPrice(
    trdk::Security &,
    const Currency &,
    const Qty &,
    const OrderParams &,
    const OrderStatusUpdateSlot &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendSell(trdk::Security &,
                                     const Currency &,
                                     const Qty &,
                                     const Price &,
                                     const OrderParams &,
                                     const OrderStatusUpdateSlot &&) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendSellImmediatelyOrCancel(
    trdk::Security &,
    const Currency &,
    const Qty &,
    const Price &,
    const OrderParams &,
    const OrderStatusUpdateSlot &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendSellAtMarketPriceImmediatelyOrCancel(
    trdk::Security &,
    const Currency &,
    const Qty &,
    const OrderParams &,
    const OrderStatusUpdateSlot &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendBuyAtMarketPrice(
    trdk::Security &,
    const Currency &,
    const Qty &,
    const OrderParams &,
    const OrderStatusUpdateSlot &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendBuy(trdk::Security &,
                                    const Currency &,
                                    const Qty &,
                                    const Price &,
                                    const OrderParams &,
                                    const OrderStatusUpdateSlot &&) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendBuyImmediatelyOrCancel(
    trdk::Security &,
    const Currency &,
    const Qty &,
    const Price &,
    const OrderParams &,
    const OrderStatusUpdateSlot &) {
  throw MethodDoesNotImplementedError("Methods is not supported");
}

OrderId fix::TradingSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
    trdk::Security &,
    const Currency &,
    const Qty &,
    const OrderParams &,
    const OrderStatusUpdateSlot &) {
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
