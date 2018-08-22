/*******************************************************************************
 *   Created: 2018/03/10 02:37:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Operation.hpp"
#include "PnlContainer.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace Lib;
using namespace Strategies::TriangularArbitrage;

namespace ta = Strategies::TriangularArbitrage;

ta::Operation::Operation(Strategy &strategy,
                         const Opportunity::Targets &targets)
    : Base(strategy, boost::make_unique<PnlContainer>()), m_targets(targets) {}

const Opportunity::Target &ta::Operation::GetTarget(
    const Security &security) const {
  for (const auto &target : m_targets) {
    if (target.security == &security) {
      return target;
    }
  }
  Assert(false);
  throw LogicError("Failed to find leg target by security");
}

LegPolicy &ta::Operation::GetLeg(const Security &security) {
  for (size_t i = 0; i < m_targets.size(); ++i) {
    if (m_targets[i].security == &security) {
      return *boost::polymorphic_downcast<const Strategy *>(&GetStrategy())
                  ->GetLegs()[i];
    }
  }
  Assert(false);
  throw LogicError("Failed to find leg by security");
}

const LegPolicy &ta::Operation::GetLeg(const Security &security) const {
  return const_cast<Operation *>(this)->GetLeg(security);
}

const TradingLib::OrderPolicy &ta::Operation::GetOpenOrderPolicy(
    const Position &) const {
  return m_orderPolicy;
}

const TradingLib::OrderPolicy &ta::Operation::GetCloseOrderPolicy(
    const Position &) const {
  return m_orderPolicy;
}

bool ta::Operation::IsLong(const Security &security) const {
  return GetLeg(security).GetSide() == ORDER_SIDE_LONG;
}

Qty ta::Operation::GetPlannedQty(const Security &security) const {
  return GetTarget(security).qty;
}

bool ta::Operation::HasCloseSignal(const Position &) const { return false; }
