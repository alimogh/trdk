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

namespace trdk {
namespace Strategies {
namespace MrigeshKejriwal {

class OrderPolicy : public TradingLib::LimitIocOrderPolicy {
 public:
  typedef TradingLib::LimitIocOrderPolicy Base;

 public:
  explicit OrderPolicy(const Price &correction);
  virtual ~OrderPolicy() override = default;

 protected:
  virtual trdk::ScaledPrice GetOpenOrderPrice(trdk::Position &) const override;
  virtual trdk::ScaledPrice GetCloseOrderPrice(trdk::Position &) const override;

 private:
  const Price m_correction;
};
}
}
}