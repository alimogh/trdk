/*******************************************************************************
 *   Created: 2018/02/21 15:49:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TakerController.hpp"

using namespace trdk;
using namespace TradingLib;
using namespace Strategies::MarketMaker;

void TakerController::OnPositionUpdate(Position &position) {
  if (position.GetSubOperationId() != 1) {
    if (position.IsCompleted()) {
      return;
    }
    if (position.GetNumberOfOrders() && !position.HasActiveOrders()) {
      position.MarkAsCompleted();
      return;
    }
  } else if (position.IsCompleted()) {
    for (auto &secondLeg : position.GetStrategy().GetPositions()) {
      if (&secondLeg == &position ||
          &*secondLeg.GetOperation() != &*position.GetOperation()) {
        continue;
      }
      AssertEq(2, secondLeg.GetSubOperationId());
      if (!secondLeg.IsCompleted()) {
        if (secondLeg.HasActiveOrders()) {
          secondLeg.CancelAllOrders();
        } else {
          secondLeg.MarkAsCompleted();
        }
      }
    }
  }
  Base::OnPositionUpdate(position);
}

void TakerController::HoldPosition(Position &position) {
  ClosePosition(position, CLOSE_REASON_TAKE_PROFIT);
}
