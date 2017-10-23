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
#include "TradingSystem.hpp"
#include "Core/Security.hpp"
#include "IncomingMessages.hpp"
#include "OutgoingMessages.hpp"

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
  throw MethodIsNotImplementedException("Methods is not supported");
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

OrderId fix::TradingSystem::SendSellImmediatelyOrCancel(
    trdk::Security &security,
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

OrderId fix::TradingSystem::SendSellAtMarketPriceImmediatelyOrCancel(
    trdk::Security &, const Currency &, const Qty &, const OrderParams &) {
  throw MethodIsNotImplementedException("Methods is not supported");
}

OrderId fix::TradingSystem::SendBuyAtMarketPrice(trdk::Security &,
                                                 const Currency &,
                                                 const Qty &,
                                                 const OrderParams &) {
  throw MethodIsNotImplementedException("Methods is not supported");
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

OrderId fix::TradingSystem::SendBuyImmediatelyOrCancel(trdk::Security &security,
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

OrderId fix::TradingSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
    trdk::Security &, const Currency &, const Qty &, const OrderParams &) {
  throw MethodIsNotImplementedException("Methods is not supported");
}

void fix::TradingSystem::SendCancelOrder(const OrderId &) {
  throw MethodIsNotImplementedException("Methods is not supported");
}

void fix::TradingSystem::OnConnectionRestored() {
  throw MethodIsNotImplementedException(
      "fix::TradingSystem::OnConnectionRestored is not implemented");
}

void fix::TradingSystem::OnReject(const in::Reject &message,
                                  Lib::NetworkStreamClient &client) {
  const auto &orderId = message.ReadRefSeqNum();
  try {
    OnOrderReject(orderId, std::string(), message.ReadText());
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
  try {
    OnOrderReject(message.ReadBusinessRejectRefId(), std::string(),
                  std::move(reason));
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
