/*******************************************************************************
 *   Created: 2018/02/21 15:49:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TakerOperation.hpp"

using namespace trdk;
using namespace TradingLib;
using namespace Strategies::MarketMaker;

PnlContainer::Result TakerOperation::PnlContainer::GetResult() const {
  const auto &result = Base::GetResult();
  return result == RESULT_ERROR ? RESULT_LOSS : result;
}

Price TakerOperation::OrderPolicy::GetOpenOrderPrice(Position &position) const {
  if (position.GetSubOperationId() != 1) {
    for (const auto &firstLeg : position.GetStrategy().GetPositions()) {
      if (firstLeg.GetSubOperationId() != 1 ||
          &*firstLeg.GetOperation() != &*position.GetOperation()) {
        continue;
      }
      return *firstLeg.GetActiveCloseOrderPrice();
    }
  }
  return position.GetMarketOpenPrice() * (position.IsLong() ? 1.05 : 0.95);
}

Price TakerOperation::OrderPolicy::GetCloseOrderPrice(
    Position &position) const {
  AssertEq(1, position.GetSubOperationId());
  return position.GetOpenAvgPrice() * (position.IsLong() ? 1.0005 : 0.9995);
}

TakerOperation::TakerOperation(Strategy &strategy,
                               const bool isLong,
                               const Qty &qty)
    : Base(strategy, boost::make_unique<PnlContainer>()),
      m_isLong(isLong),
      m_qty(qty) {}

const OrderPolicy &TakerOperation::GetOpenOrderPolicy(const Position &) const {
  return m_orderPolicy;
}

const OrderPolicy &TakerOperation::GetCloseOrderPolicy(const Position &) const {
  return m_orderPolicy;
}

bool TakerOperation::OnCloseReasonChange(Position &position,
                                         const CloseReason &reason) {
  if (position.HasActiveOrders() && !position.IsCancelling()) {
    position.CancelAllOrders();
  }
  return Base::OnCloseReasonChange(position, reason);
}

boost::shared_ptr<Operation> TakerOperation::StartInvertedPosition(
    const Position &) {
  return nullptr;
}

bool TakerOperation::IsLong(const Security &) const { return m_isLong; }

Qty TakerOperation::GetPlannedQty(const Security &) const { return m_qty; }

bool TakerOperation::HasCloseSignal(const Position &) const { return false; }
