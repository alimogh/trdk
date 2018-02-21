/*******************************************************************************
 *   Created: 2017/09/07 13:58:33
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace TradingLib {

class OrderPolicy : private boost::noncopyable {
 public:
  virtual ~OrderPolicy() = default;

 public:
  virtual void Open(trdk::Position &) const = 0;
  virtual void Close(trdk::Position &) const = 0;
};

class LimitOrderPolicy : public trdk::TradingLib::OrderPolicy {
 public:
  virtual ~LimitOrderPolicy() override = default;

 public:
  virtual trdk::Price GetOpenOrderPrice(trdk::Position &) const;
  virtual trdk::Price GetCloseOrderPrice(trdk::Position &) const;
};

class LimitGtcOrderPolicy : public trdk::TradingLib::LimitOrderPolicy {
 public:
  virtual ~LimitGtcOrderPolicy() override = default;

 public:
  virtual void Open(trdk::Position &) const override;
  virtual void Close(trdk::Position &) const override;
};

class LimitPriceGtcOrderPolicy : public trdk::TradingLib::LimitGtcOrderPolicy {
 public:
  explicit LimitPriceGtcOrderPolicy(const trdk::Price &price)
      : m_price(price) {}
  virtual ~LimitPriceGtcOrderPolicy() override = default;

 public:
  virtual trdk::Price GetOpenOrderPrice(trdk::Position &) const override {
    return m_price;
  }
  virtual trdk::Price GetCloseOrderPrice(trdk::Position &) const override {
    return m_price;
  }

 private:
  const trdk::Price m_price;
};

class LimitIocOrderPolicy : public trdk::TradingLib::LimitOrderPolicy {
 public:
  virtual ~LimitIocOrderPolicy() override = default;

 public:
  virtual void Open(trdk::Position &) const override;
  virtual void Close(trdk::Position &) const override;
};

class MarketGtcOrderPolicy : public trdk::TradingLib::OrderPolicy {
 public:
  virtual ~MarketGtcOrderPolicy() override = default;

 public:
  virtual void Open(trdk::Position &) const override;
  virtual void Close(trdk::Position &) const override;
};
}  // namespace TradingLib
}  // namespace trdk