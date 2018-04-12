/*******************************************************************************
 *   Created: 2017/08/26 19:08:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PositionController.hpp"
#include "OrderPolicy.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
using namespace TradingLib;

namespace pt = boost::posix_time;

namespace {
boost::optional<OrderCheckError> CheckOrder(const Position& position,
                                            const Qty& qty,
                                            const Price& price,
                                            const OrderSide& side) {
  return position.GetTradingSystem().CheckOrder(
      position.GetSecurity(), position.GetCurrency(), qty, price, side);
}

boost::optional<OrderCheckError> CheckOpening(const Position& position) {
  AssertGt(position.GetPlanedQty(), position.GetOpenedQty());
  return CheckOrder(position, position.GetPlanedQty() - position.GetOpenedQty(),
                    position.GetMarketOpenPrice(), position.GetOpenOrderSide());
}

boost::optional<OrderCheckError> CheckClosing(const Position& position) {
  return CheckOrder(position, position.GetActiveQty(),
                    position.GetMarketClosePrice(),
                    position.GetCloseOrderSide());
}
}  // namespace

Position* PositionController::Open(
    const boost::shared_ptr<Operation>& operationContext,
    const int64_t subOperationId,
    Security& security,
    const Milestones& delayMeasurement) {
  return Open(operationContext, subOperationId, security,
              operationContext->IsLong(security), delayMeasurement);
}

Position* PositionController::Open(
    const boost::shared_ptr<Operation>& operationContext,
    const int64_t subOperationId,
    Security& security,
    const bool isLong,
    const Milestones& delayMeasurement) {
  return Open(operationContext, subOperationId, security, isLong,
              operationContext->GetPlannedQty(security), delayMeasurement);
}

Position* PositionController::Open(
    const boost::shared_ptr<Operation>& operationContext,
    const int64_t subOperationId,
    Security& security,
    const bool isLong,
    const Qty& qty,
    const Milestones& delayMeasurement) {
  const auto result =
      Create(operationContext, subOperationId, isLong, security, qty,
             isLong ? security.GetAskPrice() : security.GetBidPrice(),
             delayMeasurement);
  // Will not check order requirements as this is orders initiated by the
  // client, the controller only checks automated next orders.
  Continue(*result);
  return &*result;
}

Position& PositionController::Restore(
    const boost::shared_ptr<Operation>& operationContext,
    const int64_t subOperationId,
    Security& security,
    const bool isLong,
    const pt::ptime& openStartTime,
    const pt::ptime& openTime,
    const Qty& qty,
    const Price& startPrice,
    const Price& openPrice,
    const boost::shared_ptr<const OrderTransactionContext>& openingContext,
    const Milestones& delayMeasurement) {
  auto result = Create(operationContext, subOperationId, isLong, security, qty,
                       startPrice, delayMeasurement);
  result->RestoreOpenState(openStartTime, openTime, openPrice, openingContext);
  return *result;
}

void PositionController::Continue(Position& position) {
  Assert(!position.HasActiveOrders());
  position.GetOperation()->GetOpenOrderPolicy(position).Open(position);
}

void PositionController::Hold(Position& position) {
  AssertLt(0, position.GetActiveQty());
  Assert(position.IsFullyOpened());
  Assert(!position.IsCompleted());
  UseUnused(position);
}
void PositionController::Hold(Position& position, const OrderCheckError&) {
  AssertLt(0, position.GetActiveQty());
  Assert(!position.IsFullyOpened());
  Continue(position);
}

void PositionController::Complete(Position& position) {
  AssertEq(0, position.GetActiveQty());
  Assert(position.IsCompleted());
  UseUnused(position);
}
void PositionController::Complete(Position& position, const OrderCheckError&) {
  AssertLt(0, position.GetActiveQty());
  Assert(!position.IsCompleted());
  Close(position);
}

bool PositionController::Close(Position& position, const CloseReason& reason) {
  IsPassive(reason) ? position.SetCloseReason(reason)
                    : position.ResetCloseReason(reason);
  if (position.IsCompleted()) {
    Assert(!position.HasActiveOpenOrders());
    AssertEq(0, position.GetActiveQty());
    Complete(position);
    return false;
  }
  if (position.HasActiveOpenOrders()) {
    if (position.IsCancelling()) {
      return false;
    }
    try {
      Verify(position.CancelAllOrders());
      return true;
    } catch (const OrderIsUnknownException&) {
      return false;
    }
  }
  if (position.HasActiveCloseOrders()) {
    return false;
  }
  // Will not check order requirements as this is orders initiated by the
  // client, the controller only checks automated next orders.
  Close(position);
  return true;
}

void PositionController::Close(Position& position) {
  Assert(!position.HasActiveOrders());
  const auto& policy = position.GetOperation()->GetCloseOrderPolicy(position);
  try {
    policy.Close(position);
  } catch (const CommunicationError& ex) {
    position.GetStrategy().GetLog().Warn(
        R"(Failed to close position "%1%/%2%": "%3%".)",
        position.GetOperation()->GetId(),  // 1
        position.GetSubOperationId(),      // 2
        ex);                               // 3
    throw;
  }
}

void PositionController::CloseAll(Strategy& strategy,
                                  const CloseReason& reason) {
  for (auto& position : strategy.GetPositions()) {
    // Will not check order requirements as this is orders initiated by the
    // client, the controller only checks automated next orders.
    try {
      Close(position, reason);
    } catch (const std::exception& ex) {
      strategy.GetLog().Error(
          "Failed to close position %1%/%2% with reason %3%: \"%4%\".",
          position.GetOperation()->GetId(),  // 1
          position.GetSubOperationId(),      // 2
          reason,                            // 3
          ex.what());                        // 4
    }
  }
}

Position* PositionController::OnSignal(
    const boost::shared_ptr<Operation>& newOperationContext,
    const int64_t subOperationId,
    Security& security,
    const Milestones& delayMeasurement) {
  auto& strategy = newOperationContext->GetStrategy();
  auto* position = GetExisting(strategy, security);
  if (position) {
    if (position->IsCompleted()) {
      Complete(*position);
      position = nullptr;
    } else if (!position->GetOperation()->HasCloseSignal(*position)) {
      if (!position->HasActiveCloseOrders()) {
        position->ResetCloseReason();
      }
      delayMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
      return nullptr;
    }
  }

  if (position &&
      (position->IsCancelling() || position->HasActiveCloseOrders())) {
    return nullptr;
  }

  delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_1);

  if (!position) {
    position =
        Open(newOperationContext, subOperationId, security, delayMeasurement);
    if (!position) {
      return nullptr;
    }
  } else if (!Close(*position, CLOSE_REASON_SIGNAL)) {
    return nullptr;
  }

  delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);

  return position;
}

void PositionController::OnUpdate(Position& position) {
  AssertLt(0, position.GetNumberOfOpenOrders());

  if (position.IsCompleted()) {
    if (position.GetActiveQty() > 0 || position.HasActiveOrders()) {
      // It seems position is marked as completed for some reason. It's not a
      // business of the general code.
      return;
    }

    // No active order, no active qty...
    if (position.GetNumberOfCloseOrders()) {
      // Position fully closed.
      Complete(position);
      auto& operation = *position.GetOperation();
      if (operation.HasCloseSignal(position)) {
        const auto& newOperation = operation.StartInvertedPosition(position);
        if (newOperation) {
          //! @todo Move StartInvertedPosition to the closing start and close
          //! position x2 with "restoring" position object as opposite position.
          Open(newOperation, position.GetSubOperationId(),
               position.GetSecurity(), !position.IsLong(), Milestones());
        }
      }
      return;
    }

    // Open order was canceled by some condition. Checking open
    // signal again and sending new open order...
    if (position.GetCloseReason() == CLOSE_REASON_NONE &&
        !position.GetOperation()->HasCloseSignal(position)) {
      // Will not check order requirements as it should be first order, the
      // controller only checks next orders.
      Continue(position);
    } else {
      Complete(position);
    }
    // Position will be deleted if was not continued.
    return;
  }

  if (position.GetNumberOfCloseOrders()) {
    // Position closing started.
    Assert(!position.HasActiveOpenOrders());
    AssertNe(CLOSE_REASON_NONE, position.GetCloseReason());
    if (position.HasActiveCloseOrders()) {
      // Closing in progress.
    } else if (position.GetCloseReason() != CLOSE_REASON_NONE) {
      // Close order was canceled by some condition. Sending
      // new close order.
      {
        const auto checkError = CheckClosing(position);
        if (checkError) {
          Complete(position, *checkError);
          return;
        }
      }
      Close(position);
    }
    return;
  }

  if (position.HasActiveOrders()) {
    // Opening in progress.
    Assert(!position.HasActiveCloseOrders());
    if (position.GetCloseReason() != CLOSE_REASON_NONE) {
      // Close signal received, closing position. Will not check order
      // requirements as this is orders initiated by the client, the controller
      // only checks automated next orders.
      Close(position);
    }
    return;
  }

  // Position is active.

  if (position.GetCloseReason() != CLOSE_REASON_NONE) {
    // Received signal to close. Will not check order requirements as this is
    // orders initiated by the client, the controller only checks automated
    // next orders.
    AssertLt(0, position.GetActiveQty());
    Close(position);
    return;
  }

  if (position.IsFullyOpened()) {
    Hold(position);
    return;
  }

  {
    const auto checkError = CheckOpening(position);
    if (checkError) {
      Hold(position, *checkError);
      return;
    }
  }
  Continue(position);
}

void PositionController::OnCloseAllRequest(Strategy& strategy) {
  CloseAll(strategy, CLOSE_REASON_REQUEST);
}

void PositionController::OnBrokerUpdate(
    const boost::shared_ptr<Operation>& operation,
    const int64_t subOperationId,
    Security& security,
    const bool isLong,
    const Qty& qty,
    const Volume& volume,
    const bool isInitial) {
  if (!isInitial || qty == 0) {
    operation->GetStrategy().GetLog().Debug(
        R"(Skipped broker position "%5%" %1% (volume %2%) for "%3%" (%4%).)",
        qty,                               // 1
        volume,                            // 2
        security,                          // 3
        isInitial ? "initial" : "online",  // 4
        isLong ? "long" : "short");        // 5
    return;
  }

  const auto& now = security.GetContext().GetCurrentTime();

  const auto price = volume / qty;
  operation->GetStrategy().GetLog().Info(
      "Accepting broker position \"%5%\" %1% (volume %2%, start price %3%) for "
      "\"%4%\"...",
      qty,                         // 1
      volume,                      // 2
      price,                       // 3
      security,                    // 4
      isLong ? "long" : "short");  // 5
  Restore(operation, subOperationId, security, isLong, now, now, qty, price,
          price, nullptr, Milestones());
}

namespace {
template <typename PositionType>
boost::shared_ptr<Position> CreateObject(
    const boost::shared_ptr<Operation>& operation,
    int64_t subOperationId,
    Security& security,
    const Qty& qty,
    const Price& price,
    const Milestones& delayMeasurement) {
  return boost::make_shared<PositionType>(operation, subOperationId, security,
                                          security.GetSymbol().GetCurrency(),
                                          qty, price, delayMeasurement);
}
}  // namespace
boost::shared_ptr<Position> PositionController::Create(
    const boost::shared_ptr<Operation>& operation,
    const int64_t subOperationId,
    const bool isLong,
    Security& security,
    const Qty& qty,
    const Price& startPrice,
    const Milestones& delayMeasurement) {
  const auto& result =
      isLong ? CreateObject<LongPosition>(operation, subOperationId, security,
                                          qty, startPrice, delayMeasurement)
             : CreateObject<ShortPosition>(operation, subOperationId, security,
                                           qty, startPrice, delayMeasurement);
  result->GetOperation()->Setup(*result, *this);
  return result;
}

Position* PositionController::GetExisting(Strategy& strategy, Security&) {
  if (strategy.GetPositions().IsEmpty()) {
    return nullptr;
  }
  AssertEq(1, strategy.GetPositions().GetSize());
  return &*strategy.GetPositions().GetBegin();
}
