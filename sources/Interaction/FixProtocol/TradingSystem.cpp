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
#include "IncomingMessages.hpp"
#include "OutgoingMessages.hpp"
#include "Policy.hpp"
#include "Settings.hpp"
#include "TransactionContext.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace Interaction::FixProtocol;
namespace fix = Interaction::FixProtocol;
namespace out = Outgoing;
namespace in = Incoming;
namespace ptr = boost::property_tree;

fix::TradingSystem::TradingSystem(const TradingMode& mode,
                                  Context& context,
                                  std::string instanceName,
                                  std::string title,
                                  const ptr::ptree& conf)
    : trdk::TradingSystem(
          mode, context, std::move(instanceName), std::move(title)),
      Handler(context, conf, trdk::TradingSystem::GetLog()),
      m_client("Trade", *this) {}

fix::TradingSystem::~TradingSystem() {
  try {
    m_client.Stop();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

Context& fix::TradingSystem::GetContext() {
  return trdk::TradingSystem::GetContext();
}

const Context& fix::TradingSystem::GetContext() const {
  return trdk::TradingSystem::GetContext();
}

ModuleEventsLog& fix::TradingSystem::GetLog() const {
  return trdk::TradingSystem::GetLog();
}

bool fix::TradingSystem::IsConnected() const { return m_client.IsConnected(); }

Volume fix::TradingSystem::CalcCommission(const Qty&,
                                          const Price&,
                                          const OrderSide&,
                                          const trdk::Security&) const {
  return 0;
}

void fix::TradingSystem::CreateConnection() {
  GetLog().Debug("Connecting...");
  m_client.Connect();
  GetLog().Info("Connected.");
}

std::unique_ptr<trdk::OrderTransactionContext>
fix::TradingSystem::SendOrderTransaction(trdk::Security& security,
                                         const Currency& currency,
                                         const Qty& qty,
                                         const boost::optional<Price>& price,
                                         const OrderParams& params,
                                         const OrderSide& side,
                                         const TimeInForce& tif) {
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

  out::NewOrderSingle message(security, side, qty, *price,
                              GetStandardOutgoingHeader());
  if (params.stopPrice) {
    message.SetStopPx(*params.stopPrice);
  }
  if (params.position) {
    message.SetPosMaintRptId(
        boost::polymorphic_downcast<const OrderTransactionContext*>(
            params.position)
            ->GetPositionId());
  }
  m_client.Send(message);
  return boost::make_unique<OrderTransactionContext>(
      *this, message.GetSequenceNumber());
}

void fix::TradingSystem::SendCancelOrderTransaction(
    const trdk::OrderTransactionContext& transaction) {
  m_client.Send(out::OrderCancelRequest(transaction.GetOrderId(),
                                        GetStandardOutgoingHeader()));
}

void fix::TradingSystem::OnConnectionRestored() {
  throw MethodIsNotImplementedException(
      "fix::TradingSystem::OnConnectionRestored is not implemented");
}

void fix::TradingSystem::OnReject(const in::Reject& message,
                                  NetworkStreamClient&) {
  const auto& orderId = message.ReadRefSeqNum();
  const auto& text = message.ReadText();
  if (GetSettings().policy->IsUnknownOrderIdError(text)) {
    GetLog().Warn("Received Reject for unknown order %1%: \"%2%\" .",
                  orderId,  // 1
                  text);    // 2
    return;
  }
  try {
    if (GetSettings().policy->IsBadTradingVolumeError(text)) {
      GetLog().Error("Received Reject for order %1%: \"%2%\" .",
                     orderId,  // 1
                     text);    // 2
      OnOrderCanceled(message.GetTime(), orderId, boost::none, boost::none);
    } else {
      OnOrderError(message.GetTime(), orderId, boost::none, boost::none,
                   std::move(text));
    }
  } catch (const OrderIsUnknownException& ex) {
    message.ResetReadingState();
    GetLog().Warn("Received Reject for unknown order %1% (\"%2%\"): \"%3%\" .",
                  orderId,              // 1
                  ex.what(),            // 2
                  message.ReadText());  // 3
  }
}

void fix::TradingSystem::OnBusinessMessageReject(
    const in::BusinessMessageReject& message,
    NetworkStreamClient&,
    const Milestones&) {
  const auto& reason = message.ReadText();
  const auto& orderId = message.ReadBusinessRejectRefId();
  if (GetSettings().policy->IsUnknownOrderIdError(reason)) {
    GetLog().Warn(
        "Received Business Message Reject for unknown order %1%: \"%2%\" .",
        orderId,  // 1
        reason);  // 2
    return;
  }
  try {
    if (GetSettings().policy->IsBadTradingVolumeError(reason)) {
      GetLog().Error(
          "Received Business Message Reject for order %1%: \"%2%\" .",
          orderId,  // 1
          reason);  // 2
      OnOrderCanceled(message.GetTime(), orderId, boost::none, boost::none);
    } else {
      OnOrderError(message.GetTime(), orderId, boost::none, boost::none,
                   std::move(reason));
    }
  } catch (const OrderIsUnknownException& ex) {
    message.ResetReadingState();
    GetLog().Warn(
        "Received Business Message Reject for unknown order %1% (\"%2%\"): "
        "\"%3%\" .",
        orderId,              // 1
        ex.what(),            // 2
        message.ReadText());  // 3
  }
}

void fix::TradingSystem::OnExecutionReport(const in::ExecutionReport& message,
                                           NetworkStreamClient&,
                                           const Milestones&) {
  const OrderId& orderId = message.ReadClOrdId();
  OrderStatus orderStatus = message.ReadOrdStatus();
  const ExecType& execType = message.ReadExecType();
  const auto& setPositionId =
      [&message](trdk::OrderTransactionContext& transactionContext) {
        boost::polymorphic_downcast<OrderTransactionContext*>(
            &transactionContext)
            ->SetPositionId(message.ReadPosMaintRptId());
        return true;
      };
  try {
    static_assert(numberOfOrderStatuses == 7, "List changed.");
    switch (execType) {
      default:
        GetLog().Warn(
            "Received Execution Report with unknown status %1% (for order "
            "%1%).",
            execType,  // 1
            orderId);  // 2
        break;
      case EXEC_TYPE_NEW:
        OnOrderOpened(message.GetTime(), orderId, setPositionId);
        break;
      case EXEC_TYPE_CANCELED:
      case EXEC_TYPE_REPLACE:
        OnOrderCanceled(message.GetTime(), orderId, message.ReadLeavesQty(),
                        boost::none);
        break;
      case EXEC_TYPE_REJECTED:
      case EXEC_TYPE_EXPIRED:
        message.ResetReadingState();
        OnOrderRejected(message.GetTime(), orderId, boost::none, boost::none);
        break;
      case EXEC_TYPE_TRADE: {
        Assert(orderStatus == ORDER_STATUS_FILLED_PARTIALLY ||
               orderStatus == ORDER_STATUS_FILLED_FULLY);
        message.ResetReadingState();
        Trade trade = {message.ReadAvgPx()};
        if (orderStatus == ORDER_STATUS_FILLED_FULLY) {
          OnOrderFilled(message.GetTime(), orderId, std::move(trade),
                        boost::none, setPositionId);
        } else {
          OnTrade(message.GetTime(), orderId, std::move(trade), setPositionId);
        }
        break;
      }
      case EXEC_TYPE_ORDER_STATUS:
        switch (orderStatus) {
          case ORDER_STATUS_OPENED:
            OnOrderOpened(message.GetTime(), orderId, setPositionId);
            break;
          case ORDER_STATUS_CANCELED:
            OnOrderCanceled(message.GetTime(), orderId, message.ReadLeavesQty(),
                            boost::none);
            break;
          case ORDER_STATUS_FILLED_FULLY:
          case ORDER_STATUS_FILLED_PARTIALLY: {
            message.ResetReadingState();
            Trade trade = {message.ReadAvgPx()};
            if (orderStatus == ORDER_STATUS_FILLED_FULLY) {
              OnOrderFilled(message.GetTime(), orderId, std::move(trade),
                            message.ReadLeavesQty(), setPositionId);
            } else {
              OnTrade(message.GetTime(), orderId, std::move(trade),
                      setPositionId);
            }
            break;
          }
          case ORDER_STATUS_REJECTED:
            message.ResetReadingState();
            OnOrderRejected(message.GetTime(), orderId, boost::none,
                            boost::none);
            break;
          case ORDER_STATUS_ERROR:
            message.ResetReadingState();
            OnOrderError(message.GetTime(), orderId, boost::none, boost::none,
                         message.ReadText());
            break;
        }
    }
  } catch (const OrderIsUnknownException& ex) {
    GetLog().Warn(
        "Received Execution Report for unknown order %1% (\"%2%\"): %3%.",
        orderId,    // 1
        ex.what(),  // 2
        execType);  // 3
  }
}

void fix::TradingSystem::OnOrderCancelReject(
    const in::OrderCancelReject& message,
    NetworkStreamClient&,
    const Milestones&) {
  GetLog().Warn("Received Order Cancel Reject for order %1%.",
                message.ReadClOrdId());
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<trdk::TradingSystem> CreateTradingSystem(
    const TradingMode& mode,
    Context& context,
    std::string instanceName,
    std::string title,
    const ptr::ptree& configuration) {
  const auto& result = boost::make_shared<fix::TradingSystem>(
      mode, context, std::move(instanceName), std::move(title), configuration);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
