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
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace pt = boost::posix_time;
namespace tl = trdk::TradingLib;
namespace aa = trdk::Strategies::ArbitrageAdvisor;

////////////////////////////////////////////////////////////////////////////////

aa::PositionController::PositionController(Strategy &strategy)
    : Base(strategy),
      m_report(boost::make_unique<BusinessOperationReport>(GetStrategy())) {}

void aa::PositionController::OnPositionUpdate(Position &position) {
  if (IsBusinessPosition(position) && position.IsCompleted()) {
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
  Assert(!position.HasActiveOrders());

  if (!IsBusinessPosition(position)) {
    Base::HoldPosition(position);
    return;
  }

  Position *const oppositePosition = FindOppositePosition(position);
  if (oppositePosition && !oppositePosition->IsFullyOpened()) {
    // Waiting until another leg will be completed.
    return;
  }

  // Operation is completed.
  CompleteBusinessOperation(position, oppositePosition);
}

void PositionController::ClosePosition(Position &position) {
  Assert(!position.HasActiveOrders());

  if (!IsBusinessPosition(position)) {
    Base::ClosePosition(position);
    return;
  }

  if (position.IsStarted() && !position.IsCompleted()) {
    AssertLt(0, position.GetActiveQty());
    CompleteBusinessOperation(position, nullptr);
  }
}

Position *aa::PositionController::FindOppositePosition(
    const Position &position) {
  Assert(IsBusinessPosition(position));
  for (auto &result : GetStrategy().GetPositions()) {
    if (&result == &position ||
        result.GetOperation() != position.GetOperation()) {
      Assert(IsBusinessPosition(position));
      continue;
    }
    Assert(IsBusinessPosition(result));
    return &result;
  }
  // Opposite position already is closed.
  return nullptr;
}

void aa::PositionController::CompleteBusinessOperation(
    Position &lastLeg, Position *legBeforeLast) {
  Assert(IsBusinessPosition(lastLeg));
  Assert(!legBeforeLast || IsBusinessPosition(lastLeg));

  lastLeg.MarkAsCompleted();
  if (legBeforeLast) {
    legBeforeLast->MarkAsCompleted();
  }
}

////////////////////////////////////////////////////////////////////////////////

PositionAndBalanceController::PositionAndBalanceController(
    aa::Strategy &strategy)
    : Base(strategy) {}

void PositionAndBalanceController::OnPositionUpdate(Position &position) {
  if (!IsBusinessPosition(position) && !position.IsCompleted() &&
      !position.HasActiveOrders() && position.GetNumberOfCloseOrders()) {
    Assert(!position.HasActiveOpenOrders());
    AssertEq(CLOSE_REASON_SIGNAL, position.GetCloseReason());
    // Order canceled by outside.
    position.MarkAsCompleted();
    GetReport().Append(position);
    return;
  }
  Base::OnPositionUpdate(position);
}

void PositionAndBalanceController::CompleteBusinessOperation(
    Position &lastLeg, Position *legBeforeLast) {
  Base::CompleteBusinessOperation(lastLeg, legBeforeLast);

  Assert(IsBusinessPosition(lastLeg));
  Assert(!legBeforeLast || IsBusinessPosition(*legBeforeLast));
  Assert(lastLeg.IsCompleted());
  Assert(!legBeforeLast || legBeforeLast->IsCompleted());

  RestoreBalance(lastLeg, legBeforeLast);
  if (legBeforeLast) {
    RestoreBalance(*legBeforeLast, &lastLeg);
  }
}

void PositionAndBalanceController::RestoreBalance(Position &position,
                                                  Position *oppositePosition) {
  AssertLe(1, position.GetSubOperationId());
  AssertGe(2, position.GetSubOperationId());

  if (!position.GetActiveQty()) {
    return;
  }

  pt::ptime openStartTime;
  pt::ptime openTime;
  Price startPrice;
  Price openPrice;
  if (oppositePosition && oppositePosition->GetActiveQty()) {
    openStartTime = oppositePosition->GetOpenStartTime();
    openTime = oppositePosition->GetOpenTime();
    startPrice = oppositePosition->GetOpenStartPrice();
    openPrice = oppositePosition->GetOpenAvgPrice();
  } else {
    openStartTime = position.GetOpenStartTime();
    openTime = position.GetOpenTime();
    startPrice = position.GetOpenStartPrice();
    openPrice = position.GetOpenAvgPrice();
  }
  Price targetPrice = openPrice;
  {
    // 0.25% is max. fee:
    const double fee = (0.25 / 100.0) * (position.IsLong() ? 1.0 : -1.0);
    targetPrice += openPrice * fee;
    targetPrice += targetPrice * fee;
  }

  auto &restoredPosition = RestorePosition(
      boost::make_shared<BalanceRestoringOperation>(position.GetOperation(),
                                                    std::move(targetPrice)),
      position.GetSubOperationId() + 2, position.GetSecurity(),
      position.IsLong(), openStartTime, openTime, position.GetActiveQty(),
      std::move(startPrice), std::move(openPrice), position.GetOpeningContext(),
      Milestones());

  try {
    ClosePosition(restoredPosition, CLOSE_REASON_SIGNAL);
  } catch (const Exception &ex) {
    GetStrategy().GetLog().Warn(
        "Failed to send \"%1%\" order to restore balance: \"%2%\".",
        position.GetSecurity(),  // 1
        ex.what());              // 2
  }
}

std::unique_ptr<tl::PositionReport> PositionAndBalanceController::OpenReport()
    const {
  return boost::make_unique<BalanceRestoreOperationReport>(GetStrategy());
}

////////////////////////////////////////////////////////////////////////////////
