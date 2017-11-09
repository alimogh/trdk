/*******************************************************************************
 *   Created: 2017/10/26 01:49:00
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "Api.h"

namespace trdk {
namespace Strategies {
namespace WilliamCarry {

class TRDK_STRATEGY_WILLIAMCARRY_API OrderPolicy
    : public TradingLib::LimitIocOrderPolicy {
 public:
  typedef LimitIocOrderPolicy Base;

 public:
  explicit OrderPolicy(const Price &maxOpenPrice)
      : m_maxOpenPrice(maxOpenPrice) {}

 protected:
  virtual trdk::Price GetOpenOrderPrice(Position &position) const override {
    return position.IsLong()
               ? std::min(Base::GetOpenOrderPrice(position), m_maxOpenPrice)
               : std::max(Base::GetOpenOrderPrice(position), m_maxOpenPrice);
  }

 private:
  const Price m_maxOpenPrice;
};
}
}
}