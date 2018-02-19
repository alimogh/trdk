/*******************************************************************************
 *   Created: 2017/11/07 00:01:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PositionController.hpp"
#include "Operation.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace aa = trdk::Strategies::ArbitrageAdvisor;

void aa::PositionController::OnPositionUpdate(Position &position) {
  auto *const oppositePosition = FindOppositePosition(position);
  if (position.IsCompleted()) {
    if (oppositePosition && !oppositePosition->IsCompleted()) {
      OnPositionUpdate(*oppositePosition);
    }
    return;
  }

  if (position.IsRejected() &&
      (position.GetNumberOfCloseOrders() == 0 ? !position.IsFullyOpened()
                                              : position.GetActiveQty() > 0) &&
      boost::istarts_with(position.GetTradingSystem().GetInstanceName(),
                          "yobit")) {
    // Special logic for Yobit.net, see also https://trello.com/c/KYVxS36k
    Assert(!position.HasActiveOrders());
    position.GetStrategy().GetLog().Error(
        "Order for position \"%1%/%2%\" (\"%3%\") is rejected by trading "
        "system \"%4%\".",
        position.GetOperation()->GetId(),  // 1
        position.GetSubOperationId(),      // 2
        position.GetSecurity(),            // 3
        position.GetTradingSystem());      // 4
    position.AddVirtualTrade(
        position.GetNumberOfCloseOrders()
            ? position.GetActiveQty()
            : position.GetPlanedQty() - position.GetActiveQty(),
        position.GetNumberOfTrades() ? position.GetLastTradePrice()
                                     : position.GetOpenStartPrice());
    Assert(!(position.GetNumberOfCloseOrders() == 0
                 ? !position.IsFullyOpened()
                 : position.GetActiveQty() > 0));
  }

  position.GetCloseReason() == CLOSE_REASON_NONE &&oppositePosition &&
          oppositePosition->GetCloseReason() != CLOSE_REASON_NONE
      ? ClosePosition(position, oppositePosition->GetCloseReason())
      : Base::OnPositionUpdate(position);
}

void aa::PositionController::HoldPosition(Position &position) {
  Assert(position.IsFullyOpened());
  Assert(!position.IsCompleted());
  Assert(!position.HasActiveOrders());

  Position *const oppositePosition = FindOppositePosition(position);
  if (!oppositePosition) {
    ClosePosition(position, CLOSE_REASON_SYSTEM_ERROR);
    return;
  }
  Assert(!position.IsCompleted());

  if (!oppositePosition->IsFullyOpened()) {
    // Waiting until another leg will be completed.
    return;
  }

  // Operation is completed.
  position.MarkAsCompleted();
  oppositePosition->MarkAsCompleted();
}

namespace {
bool PrepareOperationClose(Position &position) {
  if (!position.HasActiveOpenOrders()) {
    return true;
  }
  if (position.IsCancelling()) {
    return false;
  }
  Verify(position.CancelAllOrders());
  return false;
}

bool ChooseBestExchange(Position &position) {
  const auto &checker = BestSecurityCheckerForPosition::Create(position, true);
  boost::polymorphic_cast<aa::Strategy *>(&position.GetStrategy())
      ->ForEachSecurity(
          position.GetSecurity().GetSymbol(),
          [&checker](Security &security) { checker->Check(security); });
  if (!checker->HasSuitableSecurity()) {
    position.GetStrategy().GetLog().Error(
        "Failed to find suitable security for the position \"%1%/%2%\" (actual "
        "security is \"%3%\") to close the rest of the position %4$.8f out of "
        "%5$.8f.",
        position.GetOperation()->GetId(),  // 1
        position.GetSubOperationId(),      // 2
        position.GetSecurity(),            // 3
        position.GetOpenedQty(),           // 4
        position.GetActiveQty());          // 5
    position.MarkAsCompleted();
    return false;
  }
  return true;
}

Position *GetAbsolutePosition(Position &signalPosition) {
  Assert(!signalPosition.IsCompleted());

  auto *const oppositePosition = FindOppositePosition(signalPosition);
  if (!oppositePosition) {
    return &signalPosition;
  }
  Assert(!oppositePosition->HasActiveCloseOrders());

  if (oppositePosition->HasActiveOpenOrders()) {
    const auto listTransaction =
        signalPosition.GetStrategy().StartThreadPositionsTransaction();
    auto oppositePositionClosePreparingFuture =
        boost::async([oppositePosition]() -> bool {
          try {
            return PrepareOperationClose(*oppositePosition);
          } catch (const CommunicationError &ex) {
            throw boost::enable_current_exception(ex);
          } catch (const Exception &ex) {
            throw boost::enable_current_exception(ex);
          } catch (const std::exception &ex) {
            throw boost::enable_current_exception(ex);
          }
        });
    if (!PrepareOperationClose(signalPosition) ||
        !oppositePositionClosePreparingFuture.get()) {
      return nullptr;
    }
  } else if (!PrepareOperationClose(signalPosition)) {
    return nullptr;
  }

  Assert(!oppositePosition->HasActiveOrders());
  Assert(!signalPosition.HasActiveOrders());

  auto &longPosition =
      signalPosition.IsLong() ? signalPosition : *oppositePosition;
  auto &shortPosition =
      &longPosition == oppositePosition ? signalPosition : *oppositePosition;
  const Qty &absolutePositionSize =
      longPosition.GetActiveQty() - shortPosition.GetActiveQty();

  if (!absolutePositionSize) {
    if (!oppositePosition->IsCompleted()) {
      oppositePosition->MarkAsCompleted();
    }
    signalPosition.MarkAsCompleted();
    return false;
  }

  struct Positions {
    Position &active;
    Position &completed;
  } positions = absolutePositionSize < 0
                    ? Positions{shortPosition, longPosition}
                    : Positions{longPosition, shortPosition};
  Assert(!oppositePosition->IsCompleted() ||
         oppositePosition != &positions.active);
  positions.active.SetClosedQty(positions.completed.GetActiveQty());
  if (!positions.completed.IsCompleted()) {
    positions.completed.MarkAsCompleted();
  }
  return &positions.active;
}
}  // namespace

void aa::PositionController::ClosePosition(Position &position) {
  {
    auto *const absolutePosition = GetAbsolutePosition(position);
    if (!absolutePosition) {
      return;
    } else if (&position != absolutePosition) {
      ClosePosition(*absolutePosition, position.GetCloseReason());
      return;
    }
  }
  if (!ChooseBestExchange(position)) {
    return;
  }
  Base::ClosePosition(position);
}
