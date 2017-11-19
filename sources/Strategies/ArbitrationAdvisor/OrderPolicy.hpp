/*******************************************************************************
 *   Created: 2017/11/10 17:17:22
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
namespace ArbitrageAdvisor {

class OrderPolicy : public TradingLib::LimitGtcOrderPolicy {
 public:
  typedef LimitGtcOrderPolicy Base;

 public:
  explicit OrderPolicy(const Price &sellPrice, const Price &buyPrice);
  virtual ~OrderPolicy() override = default;

 protected:
  virtual trdk::Price GetOpenOrderPrice(trdk::Position &) const override;
  virtual trdk::Price GetCloseOrderPrice(trdk::Position &) const override;

 private:
  const Price m_sellPrice;
  const Price m_buyPrice;
};
}
}
}