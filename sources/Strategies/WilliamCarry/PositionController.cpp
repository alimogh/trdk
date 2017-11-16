/*******************************************************************************
 *   Created: 2017/11/09 12:23:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PositionController.hpp"
#include "OperationContext.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::WilliamCarry;

PositionController::PositionController(Strategy &strategy) : Base(strategy) {}

void PositionController::OnPositionUpdate(Position &position) {
  AssertLt(0, position.GetNumberOfOpenOrders());

  if (position.IsCompleted()) {
    bool isCompleted = true;
    for (const auto &strategyPosition : GetStrategy().GetPositions()) {
      if (strategyPosition.GetCloseReason() == CLOSE_REASON_STOP_LOSS) {
        continue;
      } else if (strategyPosition.GetCloseReason() ==
                     CLOSE_REASON_TAKE_PROFIT &&
                 !strategyPosition.IsCompleted()) {
        isCompleted = false;
        break;
      }
    }
    if (isCompleted) {
      for (auto &strategyPosition : GetStrategy().GetPositions()) {
        if (strategyPosition.GetCloseReason() == CLOSE_REASON_STOP_LOSS) {
          strategyPosition.CancelAllOrders();
        } else if (!strategyPosition.IsCompleted()) {
          strategyPosition.MarkAsCompleted();
        }
      }
    }
    return;
  }

  if (position.GetNumberOfCloseOrders()) {
    if (IsPassive(position.GetCloseReason())) {
      position.MarkAsCompleted();
    } else {
      ClosePosition(position);
    }
  } else if (!position.IsFullyOpened()) {
    ContinuePosition(position);
  } else {
    HoldPosition(position);
  }
}

void PositionController::OnPostionsCloseRequest() {
  {
    Qty closedQty = 0;
    Position *rootPosition = nullptr;
    for (auto &position : GetStrategy().GetPositions()) {
      if (position.GetCloseReason() == CLOSE_REASON_STOP_LOSS) {
        continue;
      } else if (position.GetCloseReason() == CLOSE_REASON_NONE ||
                 position.GetCloseReason() == CLOSE_REASON_REQUEST) {
        Assert(!rootPosition);
        rootPosition = &position;
      } else {
        closedQty += position.GetClosedQty();
      }
    }
    Assert(rootPosition);
    if (rootPosition) {
      rootPosition->SetClosedQty(closedQty);
    }
  }
  Base::OnPostionsCloseRequest();
}

void PositionController::HoldPosition(Position &position) {
  Assert(position.IsFullyOpened());
  Assert(!position.HasActiveOrders());

  OperationContext *baseOperationContex =
      dynamic_cast<OperationContext *>(&position.GetOperationContext());
  if (!baseOperationContex || !baseOperationContex->HasSubOperations()) {
    Base::HoldPosition(position);
    return;
  }
  try {
    const auto &subOperations =
        baseOperationContex->StartSubOperations(position);
    {
      const auto qty = RoundByPrecision(
          position.GetActiveQty() / subOperations.first.size(), 2);
      for (const auto &subOperation : subOperations.first) {
        ClosePosition(
            RestorePosition(
                subOperation, position.GetSecurity(), position.IsLong(), qty,
                position.GetOpenStartPrice(), position.GetOpenAvgPrice(),
                position.GetOpeningContext(), TimeMeasurement::Milestones()),
            CLOSE_REASON_TAKE_PROFIT);
      }
    }
    for (const auto &subOperation : subOperations.second) {
      ClosePosition(RestorePosition(subOperation, position.GetSecurity(),
                                    position.IsLong(), position.GetActiveQty(),
                                    position.GetOpenStartPrice(),
                                    position.GetOpenAvgPrice(),
                                    position.GetOpeningContext(),
                                    TimeMeasurement::Milestones()),
                    CLOSE_REASON_STOP_LOSS);
    }
  } catch (const std::exception &ex) {
    GetStrategy().GetLog().Error(
        "Failed to create sub-positions objects: \"%1%\".", ex.what());
    throw;
  }
}
