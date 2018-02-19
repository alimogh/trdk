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
#include "Operation.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::PingPong;

namespace pp = trdk::Strategies::PingPong;

pp::Operation::Operation(
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

const OrderPolicy &pp::Operation::GetOpenOrderPolicy(const Position &) const {
  return m_orderPolicy;
}

const OrderPolicy &pp::Operation::GetCloseOrderPolicy(const Position &) const {
  return m_orderPolicy;
}

void pp::Operation::Setup(Position &position,
                          PositionController &controller) const {
  position.AddAlgo(
      boost::make_unique<StopLossShare>(m_stopLoss, position, controller));
  position.AddAlgo(
      boost::make_unique<TakeProfitShare>(m_takeProfit, position, controller));
}

bool pp::Operation::IsLong(const Security &) const { return m_isLong; }

Qty pp::Operation::GetPlannedQty() const { return m_qty; }

bool pp::Operation::HasCloseSignal(const Position &position) const {
  const auto &isRising =
      boost::polymorphic_downcast<const pp::Strategy *>(&GetStrategy())
          ->GetTrend(position.GetSecurity())
          .IsRising();
  return !isRising || IsLong(position.GetSecurity()) == position.IsLong();
}

boost::shared_ptr<trdk::Operation> pp::Operation::StartInvertedPosition(
    const Position &) {
  return nullptr;
}
