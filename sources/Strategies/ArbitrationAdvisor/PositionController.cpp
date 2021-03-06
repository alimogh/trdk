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
#include "Strategy.hpp"

using namespace trdk;
using namespace Lib;
using namespace TradingLib;
using namespace Strategies::ArbitrageAdvisor;

namespace aa = Strategies::ArbitrageAdvisor;

void aa::PositionController::OnUpdate(Position &position) {
  auto *const oppositePosition = FindOppositePosition(position);
  if (position.IsCompleted()) {
    if (oppositePosition && !oppositePosition->IsCompleted() &&
        !oppositePosition->IsCancelling()) {
      OnUpdate(*oppositePosition);
    }
    return;
  }

  if (position.IsRejected() &&
      (position.GetNumberOfCloseOrders() == 0 ? !position.IsFullyOpened()
                                              : position.GetActiveQty() > 0) &&
      (boost::istarts_with(position.GetTradingSystem().GetInstanceName(),
                           "crex24") ||
       boost::istarts_with(position.GetTradingSystem().GetInstanceName(),
                           "yobit"))) {
    // Special logic for Yobit.net (see also https://trello.com/c/KYVxS36k) and
    // CREX24.
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
      ? Close(position, oppositePosition->GetCloseReason())
      : Base::OnUpdate(position);
}

void aa::PositionController::Hold(Position &position) {
  AssertLt(0, position.GetActiveQty());
  Assert(position.IsFullyOpened());
  Assert(!position.IsCompleted());

  auto *const oppositePosition = FindOppositePosition(position);
  if (!oppositePosition) {
    Close(position, CLOSE_REASON_SYSTEM_ERROR);
    return;
  }

  if (!oppositePosition->IsFullyOpened()) {
    // Waiting until another leg will be completed.
    return;
  }

  // Operation is completed.
  position.MarkAsCompleted();
  oppositePosition->MarkAsCompleted();
}

namespace {
void PrepareOperationClose(Position &position) {
  Assert(position.HasActiveOpenOrders());
  if (position.IsCancelling()) {
    return;
  }
  Verify(position.CancelAllOrders());
}

bool ChooseBestExchange(Position &position) {
  const auto &checker = PositionBestSecurityChecker::Create(position);
  std::vector<std::pair<const Security *, const std::string *>> checks;
  boost::polymorphic_cast<aa::Strategy *>(&position.GetStrategy())
      ->ForEachSecurity(position.GetSecurity().GetSymbol(),
                        [&checker, &checks](Security &security) {
                          const auto &checkResult = checker->Check(security);
                          checks.emplace_back(
                              &security, checkResult
                                             ? &checkResult->GetRuleNameRef()
                                             : nullptr);
                        });
  if (!checker->HasSuitableSecurity()) {
    std::ostringstream checksStr;
    for (const auto &check : checks) {
      Assert(check.first);
      Assert(check.second);
      if (&check != &checks.front()) {
        checksStr << ", ";
      }
      checksStr << *check.first << ": ";
      if (check.second) {
        checksStr << *check.second;
      } else {
        checksStr << "unknown error";
      }
    }
    position.GetStrategy().GetLog().Error(
        "Failed to find suitable security for the position \"%1%/%2%\" (actual "
        "security is \"%3%\") to close the rest of the position: %4% out of "
        "%5% (%6%).",
        position.GetOperation()->GetId(),  // 1
        position.GetSubOperationId(),      // 2
        position.GetSecurity(),            // 3
        position.GetActiveQty(),           // 4
        position.GetOpenedQty(),           // 5
        checksStr.str());                  // 6
    position.MarkAsCompleted();
    return false;
  }
  position.ReplaceTradingSystem(*checker->GetSuitableSecurity(),
                                position.GetOperation()->GetTradingSystem(
                                    *checker->GetSuitableSecurity()));
  return true;
}

Position *CheckAbsolutePosition(Position &signalPosition) {
  Assert(!signalPosition.IsCompleted());

  auto *const oppositePosition = FindOppositePosition(signalPosition);
  if (!oppositePosition || oppositePosition->IsCompleted()) {
    return &signalPosition;
  }
  Assert(!signalPosition.HasActiveCloseOrders());
  Assert(!oppositePosition->HasActiveCloseOrders());

  if (oppositePosition->HasActiveOpenOrders()) {
    if (signalPosition.HasActiveOpenOrders()) {
      const auto listTransaction =
          signalPosition.GetStrategy().StartThreadPositionsTransaction();
      auto oppositePositionClosePreparingFuture =
          boost::async([oppositePosition]() {
            try {
              PrepareOperationClose(*oppositePosition);
            } catch (const CommunicationError &ex) {
              throw boost::enable_current_exception(ex);
            } catch (const Exception &ex) {
              throw boost::enable_current_exception(ex);
            } catch (const std::exception &ex) {
              throw boost::enable_current_exception(ex);
            }
          });
      oppositePositionClosePreparingFuture.get();
      PrepareOperationClose(signalPosition);
    } else {
      PrepareOperationClose(*oppositePosition);
    }
    return nullptr;
  }
  if (signalPosition.HasActiveOpenOrders()) {
    PrepareOperationClose(signalPosition);
    return nullptr;
  }

  Assert(!oppositePosition->HasActiveOrders());
  Assert(!signalPosition.HasActiveOrders());

  auto &longPosition =
      signalPosition.IsLong() ? signalPosition : *oppositePosition;
  auto &shortPosition =
      &longPosition == oppositePosition ? signalPosition : *oppositePosition;
  const auto &absolutePositionSize =
      longPosition.GetActiveQty() - shortPosition.GetActiveQty();

  if (!absolutePositionSize) {
    if (!oppositePosition->IsCompleted()) {
      oppositePosition->MarkAsCompleted();
    }
    signalPosition.MarkAsCompleted();
    return nullptr;
  }

  // ReSharper disable CppImplicitDefaultConstructorNotAvailable
  struct Positions {
    // ReSharper restore CppImplicitDefaultConstructorNotAvailable
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

void aa::PositionController::Close(Position &position) {
  {
    auto *const absolutePosition = CheckAbsolutePosition(position);
    if (!absolutePosition) {
      return;
    }
    if (&position != absolutePosition) {
      Close(*absolutePosition, position.GetCloseReason());
      return;
    }
  }
  if (!ChooseBestExchange(position)) {
    return;
  }
  Base::Close(position);
}
