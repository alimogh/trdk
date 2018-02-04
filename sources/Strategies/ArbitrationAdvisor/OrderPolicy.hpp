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

////////////////////////////////////////////////////////////////////////////////

class OpenOrderPolicy : public TradingLib::LimitGtcOrderPolicy {
 public:
  typedef LimitGtcOrderPolicy Base;

 public:
  explicit OpenOrderPolicy(const Price &sellPrice, const Price &buyPrice);
  virtual ~OpenOrderPolicy() override = default;

 public:
  trdk::Price GetOpenOrderPrice(bool isLong) const;

  virtual trdk::Price GetOpenOrderPrice(trdk::Position &) const override;
  virtual trdk::Price GetCloseOrderPrice(trdk::Position &) const override;

 private:
  const Price m_sellPrice;
  const Price m_buyPrice;
};

////////////////////////////////////////////////////////////////////////////////

class CloseOrderPolicy : public TradingLib::LimitIocOrderPolicy {
 public:
  typedef LimitIocOrderPolicy Base;

 public:
  virtual ~CloseOrderPolicy() override = default;

 public:
  virtual Price GetOpenOrderPrice(Position &) const override;
  virtual Price GetCloseOrderPrice(Position &) const override;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace ArbitrageAdvisor
}  // namespace Strategies
}  // namespace trdk