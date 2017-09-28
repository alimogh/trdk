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

 protected:
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
}
}