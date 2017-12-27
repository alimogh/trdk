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
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace aa = trdk::Strategies::ArbitrageAdvisor;

aa::PositionController::PositionController(Strategy &strategy)
    : Base(strategy),
      m_report(boost::make_unique<OperationReport>(GetStrategy())) {}

void aa::PositionController::OnPositionUpdate(Position &position) {
  if (position.IsCompleted()) {
    auto &reportData =
        position.GetTypedOperation<aa::Operation>().GetReportData();
    if (reportData.Add(position)) {
      m_report->Append(reportData);
    }
    return;
  }

  Base::OnPositionUpdate(position);
}

void aa::PositionController::HoldPosition(Position &position) {
  Assert(position.IsFullyOpened());
  Assert(!position.IsCompleted());
  Assert(!position.HasActiveOrders());

  Position *const oppositePosition = FindOppositePosition(position);
  Assert(oppositePosition);
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
            } catch (const Interactor::CommunicationError &ex) {
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
    auto &strategy =
        *boost::polymorphic_cast<aa::Strategy *>(&position.GetStrategy());
    Security *bestSecurity = &position.GetSecurity();

    const auto &generalCheck = [&bestSecurity,
                                &position](Security &checkSecurity) {
      return bestSecurity != &checkSecurity &&
             checkSecurity.GetBidQty() >= position.GetActiveQty() &&
             checkSecurity.IsOnline();
    };
    const auto &altCheck = [&bestSecurity, &position](
        Security & /*checkSecurity*/) { return !bestSecurity->IsOnline(); };

    position.IsLong()
        ? strategy.ForEachSecurity(position.GetSecurity().GetSymbol(),
                                   [&](Security &checkSecurity) {
                                     if (generalCheck(checkSecurity) &&
                                         (bestSecurity->GetBidPrice() <
                                              checkSecurity.GetBidPrice() ||
                                          altCheck(checkSecurity))) {
                                       bestSecurity = &checkSecurity;
                                     }
                                   })
        : strategy.ForEachSecurity(position.GetSecurity().GetSymbol(),
                                   [&](Security &checkSecurity) {
                                     if (generalCheck(checkSecurity) &&
                                         (bestSecurity->GetAskPrice() >
                                              checkSecurity.GetAskPrice() ||
                                          altCheck(checkSecurity))) {
                                       bestSecurity = &checkSecurity;
                                     }
                                   });

    position.ReplaceTradingSystem(
        *bestSecurity,
        position.GetOperation()->GetTradingSystem(strategy, *bestSecurity));
  }
  Base::ClosePosition(position);
}

Position *aa::PositionController::FindOppositePosition(
    const Position &position) {
  Position *result = nullptr;
  for (auto &oppositePosition : GetStrategy().GetPositions()) {
    if (oppositePosition.GetOperation() == position.GetOperation() &&
        &oppositePosition != &position) {
      Assert(!result);
      result = &oppositePosition;
#ifndef BOOST_ENABLE_ASSERT_HANDLER
      break;
#endif
    }
  }
  return result;
}
