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
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Interaction::FixProtocol;

namespace fix = trdk::Interaction::FixProtocol;
namespace out = fix::Outgoing;
namespace in = fix::Incoming;

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
  const out::NewOrderSingle message(security, ORDER_SIDE_SELL, qty, price,
                                    GetStandardOutgoingHeader());
  m_client.Send(message);
  return message.GetSequenceNumber();
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
  const out::NewOrderSingle message(security, ORDER_SIDE_BUY, qty, price,
                                    GetStandardOutgoingHeader());
  m_client.Send(message);
  return message.GetSequenceNumber();
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

void fix::TradingSystem::OnReject(const in::Reject &message,
                                  Lib::NetworkStreamClient &client) {
  const auto &orderId = message.ReadRefSeqNum();
  GetLog().Error("Order %1% is rejected with the reason: \"%2%\".", orderId,
                 message.ReadText());
  try {
    OnOrderReject(orderId, std::string());
  } catch (const OrderIsUnknown &) {
    message.ResetReadingState();
    Handler::OnReject(message, client);
  }
}

void fix::TradingSystem::OnBusinessMessageReject(
    const in::BusinessMessageReject &message,
    NetworkStreamClient &client,
    const Milestones &delayMeasurement) {
  const auto &reason = message.ReadText();
  const auto &orderId = message.ReadBusinessRejectRefId();
  GetLog().Error("Order %1% is rejected with the reason: \"%2%\".", orderId,
                 reason);
  try {
    OnOrderReject(orderId, std::string());
  } catch (const OrderIsUnknown &) {
    message.ResetReadingState();
    Handler::OnBusinessMessageReject(message, client, delayMeasurement);
  }
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
