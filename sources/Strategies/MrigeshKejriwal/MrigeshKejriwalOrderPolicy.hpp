/*******************************************************************************
 *   Created: 2017/09/11 09:37:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "TradingLib/OrderPolicy.hpp"
#include "Core/Position.hpp"

namespace trdk {
namespace Strategies {
namespace MrigeshKejriwal {

template <typename OrderPolicyType>
class LimitOrderPolicy : public OrderPolicyType {
 public:
  typedef OrderPolicyType Base;

 public:
  explicit LimitOrderPolicy(const Price &correction)
      : m_correction(correction) {}
  virtual ~LimitOrderPolicy() override = default;

 protected:
  virtual trdk::ScaledPrice GetOpenOrderPrice(
      trdk::Position &position) const override {
    return Base::GetOpenOrderPrice(position) +
           position.GetSecurity().ScalePrice(position.IsLong() ? m_correction
                                                               : -m_correction);
  }
  virtual trdk::ScaledPrice GetCloseOrderPrice(
      trdk::Position &position) const override {
    return Base::GetCloseOrderPrice(position) +
           position.GetSecurity().ScalePrice(position.IsLong() ? -m_correction
                                                               : m_correction);
  }

 private:
  const Price m_correction;
};
}
}
}