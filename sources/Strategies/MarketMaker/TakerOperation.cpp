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

namespace pt = boost::posix_time;

OrderParams TakerOperation::AggresivePolicy::m_orderParams = {boost::none,
                                                              pt::minutes(1)};

void TakerOperation::AggresivePolicy::Open(Position &position) const {
  position.OpenImmediatelyOrCancel(GetOpenOrderPrice(position), m_orderParams);
}

void TakerOperation::AggresivePolicy::Close(trdk::Position &position) const {
  position.CloseImmediatelyOrCancel(GetCloseOrderPrice(position),
                                    m_orderParams);
}

TakerOperation::TakerOperation(Strategy &strategy)
    : Base(strategy, boost::make_unique<PnlOneSymbolContainer>()) {}

const OrderPolicy &TakerOperation::GetOpenOrderPolicy(const Position &) const {
  return m_aggresiveOrderPolicy;
}

const OrderPolicy &TakerOperation::GetCloseOrderPolicy(const Position &) const {
  return m_aggresiveOrderPolicy;
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

bool TakerOperation::HasCloseSignal(const Position &) const { return false; }
