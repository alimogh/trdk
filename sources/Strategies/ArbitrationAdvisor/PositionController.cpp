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
#include "OperationContext.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace tl = trdk::TradingLib;
namespace aa = trdk::Strategies::ArbitrageAdvisor;

aa::PositionController::PositionController(Strategy &strategy)
    : Base(strategy), m_report(boost::make_unique<Report>(GetStrategy())) {}

aa::PositionController::~PositionController() = default;

void aa::PositionController::OnPositionUpdate(Position &position) {
  if (position.IsCompleted()) {
    auto &reportData = boost::polymorphic_cast<OperationContext *>(
                           &position.GetOperationContext())
                           ->GetReportData();
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
  // It's normal. Assert maybe removed when strategy and position removing will
  // be debugged.
  AssertGe(2, GetStrategy().GetPositions().GetSize());

  Position *const oppositePosition = FindOppositePosition(position);
  if (oppositePosition && !oppositePosition->IsFullyOpened()) {
    // Waiting until another leg will be completed.
    return;
  }

  // Operation is completed.
  position.MarkAsCompleted();
  if (oppositePosition) {
    oppositePosition->MarkAsCompleted();
  }
}

Position *aa::PositionController::FindOppositePosition(
    const Position &position) {
  for (auto &result : GetStrategy().GetPositions()) {
    if (&result == &position ||
        &result.GetOperationContext() != &position.GetOperationContext()) {
      continue;
    }
    return &result;
  }
  // Opposite position already is closed.
  return nullptr;
}

void PositionController::ClosePosition(Position &position) {
  Assert(!position.HasActiveOrders());
  if (position.IsStarted() && !position.IsCompleted()) {
    GetStrategy().GetLog().Warn(
        "Unused quantity %1% detected on the \"%2%\" position %3%.",
        position.GetActiveQty(),  // 1
        position.GetSecurity(),   // 2
        position.GetId());        // 3
    AssertLt(0, position.GetActiveQty());
    position.MarkAsCompleted();
  }
}
