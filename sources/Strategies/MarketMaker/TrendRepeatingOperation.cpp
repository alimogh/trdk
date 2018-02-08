/*******************************************************************************
 *   Created: 2018/02/05 00:22:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TrendRepeatingOperation.hpp"
#include "TrendRepeatingStrategy.hpp"

using namespace trdk;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::MarketMaker;

TrendRepeatingOperation::TrendRepeatingOperation(
    Strategy &strategy,
    const Qty &qty,
    bool isLong,
    const boost::shared_ptr<TakeProfitShare::Params> &takeProfit,
    const boost::shared_ptr<StopLossShare::Params> &stopLoss)
    : Base(strategy, boost::make_unique<PnlOneSymbolContainer>()),
      m_qty(qty),
      m_isLong(isLong),
      m_takeProfit(takeProfit),
      m_stopLoss(stopLoss) {}

const OrderPolicy &TrendRepeatingOperation::GetOpenOrderPolicy(
    const Position &) const {
  return m_orderPolicy;
}

const OrderPolicy &TrendRepeatingOperation::GetCloseOrderPolicy(
    const Position &) const {
  return m_orderPolicy;
}

void TrendRepeatingOperation::Setup(Position &position,
                                    PositionController &controller) const {
  position.AddAlgo(
      boost::make_unique<StopLossShare>(m_stopLoss, position, controller));
  position.AddAlgo(
      boost::make_unique<TakeProfitShare>(m_takeProfit, position, controller));
}

bool TrendRepeatingOperation::IsLong(const Security &) const {
  return m_isLong;
}

Qty TrendRepeatingOperation::GetPlannedQty() const { return m_qty; }

bool TrendRepeatingOperation::HasCloseSignal(const Position &position) const {
  const auto &isRising =
      boost::polymorphic_downcast<const TrendRepeatingStrategy *>(
          &GetStrategy())
          ->GetTrend(position.GetSecurity())
          .IsRising();
  return !isRising || IsLong(position.GetSecurity()) == position.IsLong();
}

boost::shared_ptr<Operation> TrendRepeatingOperation::StartInvertedPosition(
    const Position &) {
  return nullptr;
}
