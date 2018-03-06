/*******************************************************************************
 *   Created: 2018/03/10 02:36:05
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Leg.hpp"
#include "OrderPolicy.hpp"

namespace trdk {
namespace Strategies {
namespace TriangularArbitrage {

class Operation : public trdk::Operation {
 public:
  typedef trdk::Operation Base;

 public:
  explicit Operation(Strategy &,
                     const boost::array<Opportunity::Target, numberOfLegs> &);
  virtual ~Operation() override = default;

 public:
  const Opportunity::Target &GetTarget(const Security &) const;

  const LegPolicy &GetLeg(const Security &) const;
  LegPolicy &GetLeg(const Security &);

 public:
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const Position &) const override;
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const Position &) const override;

  virtual bool IsLong(const Security &) const override;

  virtual Qty GetPlannedQty(const Security &) const override;

  virtual bool HasCloseSignal(const Position &) const override;

 private:
  const boost::array<Opportunity::Target, numberOfLegs> m_targets;
  const OrderPolicy m_orderPolicy;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk
