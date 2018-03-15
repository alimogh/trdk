/*******************************************************************************
 *   Created: 2018/03/10 03:52:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Strategies {
namespace TriangularArbitrage {

class OrderPolicy : public TradingLib::LimitIocOrderPolicy {
 public:
  typedef LimitIocOrderPolicy Base;

 public:
  virtual ~OrderPolicy() override = default;

 public:
  virtual Price GetOpenOrderPrice(Position &) const override;
  virtual Price GetCloseOrderPrice(Position &) const override;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk