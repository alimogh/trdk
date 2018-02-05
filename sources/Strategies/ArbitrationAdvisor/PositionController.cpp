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

aa::PositionController::PositionController(OperationReport &&report)
    : m_report(std::move(report)) {}

void aa::PositionController::OnPositionUpdate(Position &position) {
  if (position.IsCompleted()) {
    auto &reportData =
        position.GetTypedOperation<aa::Operation>().GetReportData();
    if (reportData.Add(position)) {
      m_report.Append(reportData);
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
    return;
  }

  Base::OnPositionUpdate(position);
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

bool aa::PositionController::ClosePosition(Position &position,
                                           const CloseReason &reason) {
  boost::shared_future<void> oppositePositionClosingFuture;
  std::unique_ptr<const Strategy::PositionListTransaction> listTransaction;
  {
    auto *const oppositePosition = FindOppositePosition(position);
    if (oppositePosition &&
        oppositePosition->GetCloseReason() == CLOSE_REASON_NONE) {
      listTransaction =
          position.GetStrategy().StartThreadPositionsTransaction();
      oppositePositionClosingFuture =
          boost::async([this, &oppositePosition, &reason] {
            try {
              Base::ClosePosition(*oppositePosition, reason);
            } catch (const CommunicationError &ex) {
              throw boost::enable_current_exception(ex);
            } catch (const Exception &ex) {
              throw boost::enable_current_exception(ex);
            } catch (const std::exception &ex) {
              throw boost::enable_current_exception(ex);
            }
          });
    }
  }
  const auto &result = Base::ClosePosition(position, reason);

  if (oppositePositionClosingFuture.valid()) {
    oppositePositionClosingFuture.get();
  }
  return result;
}

void aa::PositionController::ClosePosition(Position &position) {
  {
    const auto &checker = BestSecurityCheckerForPosition::Create(position);
    boost::polymorphic_cast<aa::Strategy *>(&position.GetStrategy())
        ->ForEachSecurity(
            position.GetSecurity().GetSymbol(),
            [&checker](Security &security) { checker->Check(security); });
    if (!checker->HasSuitableSecurity()) {
      position.GetStrategy().GetLog().Error(
          "Failed to find suitable security for the position \"%1%/%2%\" "
          "(actual security is \"%3%\") to close the rest of the position "
          "%4$.8f out of %5$.8f.",
          position.GetOperation()->GetId(),  // 1
          position.GetSubOperationId(),      // 2
          position.GetSecurity(),            // 3
          position.GetOpenedQty(),           // 4
          position.GetActiveQty());          // 5
      position.MarkAsCompleted();
      return;
    }
  }
  Base::ClosePosition(position);
}
