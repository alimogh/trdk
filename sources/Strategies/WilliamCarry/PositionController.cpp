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

namespace {
bool IsRoot(const Position &position) {
  return position.GetOperationContext().GetParent() ? false : true;
}

PositionOperationContext &GetRootContext(Position &position) {
  auto *const result = position.GetOperationContext().GetParent()
                           ? position.GetOperationContext().GetParent()
                           : &position.GetOperationContext();
  Assert(result);
  if (!result) {
    throw LogicError("Failed to find positions root");
  }
  return *result;
}

std::vector<Position *> GetRootPositions(Strategy &strategy) {
  std::vector<Position *> result;
  for (auto &position : strategy.GetPositions()) {
    if (IsRoot(position)) {
      result.emplace_back(&position);
    }
  }
  return result;
}

template <typename Callback>
void ForEachChild(Position &anyPosition,
                  Strategy &strategy,
                  const Callback &callback) {
  const auto &rootContext = GetRootContext(anyPosition);
  for (auto &position : strategy.GetPositions()) {
    if (IsRoot(position) || &rootContext != &GetRootContext(position)) {
      continue;
    }
    if (!callback(position)) {
      break;
    }
  }
}
}

PositionController::PositionController(Strategy &strategy) : Base(strategy) {}

void PositionController::OnPositionUpdate(Position &position) {
  AssertLt(0, position.GetNumberOfOpenOrders());

  if (position.IsCompleted()) {
    bool isCompleted = true;
    ForEachChild(
        position, GetStrategy(),
        [&isCompleted](Position &strategyPosition) -> bool {
          if (strategyPosition.GetCloseReason() == CLOSE_REASON_TAKE_PROFIT &&
              !strategyPosition.IsCompleted()) {
            isCompleted = false;
          }
          return isCompleted;
        });
    if (isCompleted) {
      ForEachChild(
          position, GetStrategy(), [](Position &strategyPosition) -> bool {
            if (strategyPosition.GetCloseReason() == CLOSE_REASON_STOP_LOSS) {
              if (strategyPosition.HasActiveOrders() &&
                  !strategyPosition.IsCancelling()) {
                strategyPosition.CancelAllOrders();
              }
            } else if (!strategyPosition.IsCompleted()) {
              strategyPosition.MarkAsCompleted();
            }
            return true;
          });
    }
    return;
  }

  if (position.GetNumberOfCloseOrders()) {
    if (!IsRoot(position)) {
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
  for (auto *const root : GetRootPositions(GetStrategy())) {
    Qty qty = 0;
    ForEachChild(*root, GetStrategy(), [&qty](Position &position) {
      if (position.GetCloseReason() != CLOSE_REASON_STOP_LOSS) {
        qty += position.GetClosedQty();
      }
      return true;
    });
    root->SetClosedQty(qty);
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
