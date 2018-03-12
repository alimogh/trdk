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
using namespace trdk::TradingLib;
using namespace trdk::Strategies::MarketMaker;

namespace pt = boost::posix_time;

namespace {

template <bool isClosing>
Price GetMarketPrice(const Position &position) {
  const auto &security = position.GetSecurity();
  const auto spread = security.GetAskPrice() - security.GetBidPrice();
  return position.IsLong() || isClosing ? security.GetAskPrice() + spread
                                        : security.GetBidPrice() - spread;
}
}  // namespace

Price TakerOperation::PassivePolicy::GetOpenOrderPrice(
    Position &position) const {
  return GetMarketPrice<false>(position);
}

Price TakerOperation::PassivePolicy::GetCloseOrderPrice(
    Position &position) const {
  return position.GetOpenAvgPrice() * (position.IsLong() ? 1.0005 : 0.9995);
}

OrderParams TakerOperation::AggresivePolicy::m_orderParams = {boost::none,
                                                              pt::minutes(1)};

void TakerOperation::AggresivePolicy::Open(Position &position) const {
  position.OpenImmediatelyOrCancel(GetMarketPrice<false>(position),
                                   m_orderParams);
}

void TakerOperation::AggresivePolicy::Close(trdk::Position &position) const {
  position.CloseImmediatelyOrCancel(GetMarketPrice<true>(position),
                                    m_orderParams);
}

TakerOperation::TakerOperation(Strategy &strategy, bool isLong, const Qty &qty)
    : Base(strategy, boost::make_unique<PnlOneSymbolContainer>()),
      m_isLong(isLong),
      m_qty(qty) {}

const OrderPolicy &TakerOperation::GetOpenOrderPolicy(const Position &) const {
  return m_aggresiveOrderPolicy;
}

const OrderPolicy &TakerOperation::GetCloseOrderPolicy(
    const Position &position) const {
  if (IsPassive(position.GetCloseReason())) {
    return m_passivePolicy;
  } else {
    return m_aggresiveOrderPolicy;
  }
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
