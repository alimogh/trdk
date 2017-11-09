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
    const auto &subOperations = baseOperationContex->StartSubOperations();
    AssertLt(0, subOperations.size());
    if (subOperations.empty()) {
      return;
    }
    const auto qty = position.GetActiveQty() / subOperations.size();
    for (const auto &subOperation : subOperations) {
      RestorePosition(subOperation, position.GetSecurity(), position.IsLong(),
                      qty, position.GetOpenStartPrice(),
                      position.GetOpenAvgPrice(), position.GetOpeningContext(),
                      TimeMeasurement::Milestones());
    }
  } catch (const std::exception &ex) {
    GetStrategy().GetLog().Error(
        "Failed to create sub-positions objects: \"%1%\".", ex.what());
    throw;
  }
  position.MarkAsCompleted();
}
