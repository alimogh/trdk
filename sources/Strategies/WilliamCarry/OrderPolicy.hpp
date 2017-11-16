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

class OpenOrderPolicy : public TradingLib::LimitIocOrderPolicy {
 public:
  typedef LimitIocOrderPolicy Base;

 public:
  explicit OpenOrderPolicy(const Price &maxOpenPrice)
      : m_maxOpenPrice(maxOpenPrice) {}
  virtual ~OpenOrderPolicy() override = default;

 protected:
  virtual trdk::Price GetOpenOrderPrice(Position &position) const override {
    return position.IsLong()
               ? std::min(Base::GetOpenOrderPrice(position), m_maxOpenPrice)
               : std::max(Base::GetOpenOrderPrice(position), m_maxOpenPrice);
  }

 private:
  const Price m_maxOpenPrice;
};

class StopLimitOrderPolicy : public TradingLib::LimitGtcOrderPolicy {
 public:
  explicit StopLimitOrderPolicy(const Price &price) : m_price(price) {}
  virtual ~StopLimitOrderPolicy() override = default;

 protected:
  virtual trdk::Price GetOpenOrderPrice(Position &) const { return m_price; }
  virtual trdk::Price GetCloseOrderPrice(Position &) const { return m_price; }

 private:
  const Price m_price;
};

class StopLossLimitOrderPolicy : public TradingLib::LimitOrderPolicy {
 public:
  explicit StopLossLimitOrderPolicy(const Price &price) : m_price(price) {}
  virtual ~StopLossLimitOrderPolicy() override = default;

 public:
  virtual void Open(Position &position) const override {
    position.Open(GetOpenOrderPrice(position), CreateParams(position));
  }
  virtual void Close(Position &position) const override {
    position.Close(GetCloseOrderPrice(position), CreateParams(position));
  }

 private:
  OrderParams CreateParams(const Position &position) const {
    OrderParams params;
    params.position = &*position.GetOpeningContext();
    params.stopPrice = m_price;
    return params;
  }

 private:
  const Price m_price;
};
}
}
}