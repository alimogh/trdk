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
#include "Report.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace tl = trdk::TradingLib;
namespace aa = trdk::Strategies::ArbitrageAdvisor;

aa::PositionController::PositionController(Strategy &strategy)
    : Base(strategy), m_report(boost::make_unique<Report>(GetStrategy())) {}

aa::PositionController::~PositionController() = default;

void aa::PositionController::HoldPosition(Position &position) {
  Assert(position.IsFullyOpened());
  Assert(!position.HasActiveOrders());
  // It's normal. Assert maybe removed when strategy and position removing will
  // be debugged.
  AssertGe(2, GetStrategy().GetPositions().GetSize());

  Position &oppositePosition = GetOppositePosition(position);
  if (oppositePosition.IsFullyOpened()) {
    // Operation is completed.
    position.MarkAsCompleted();
    oppositePosition.MarkAsCompleted();
    m_report->Append(oppositePosition, position);
  }

  // Waiting until another leg will be completed.
}

Position &aa::PositionController::GetOppositePosition(Position &position) {
  AssertLe(2, GetStrategy().GetPositions().GetSize());
  for (auto &strategyPosition : GetStrategy().GetPositions()) {
    if (&strategyPosition == &position ||
        &strategyPosition.GetOperationContext() !=
            &position.GetOperationContext()) {
      continue;
    }
    return strategyPosition;
  }
  // Opposite position already is closed.
  position.GetStrategy().GetLog().Error(
      "Position %1% is active, but opposite is already closed.",
      position.GetId());
  throw Lib::LogicError(
      "Trading logic error: the leg is already closed when another still "
      "be active");
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
