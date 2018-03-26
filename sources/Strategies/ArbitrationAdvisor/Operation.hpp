/*******************************************************************************
 *   Created: 2017/11/05 20:21:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "OrderPolicy.hpp"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

////////////////////////////////////////////////////////////////////////////////

class Operation : public trdk::Operation {
 public:
  typedef trdk::Operation Base;

 public:
  explicit Operation(
      trdk::Strategy &,
      Security &sellTarget,
      Security &buyTarget,
      const Qty &maxQty,
      const Price &sellPrice,
      const Price &buyPrice,
      const boost::optional<boost::posix_time::time_duration> &&stopLossDelay);
  virtual ~Operation() override = default;

 public:
  bool IsSame(const Security &sellTarget, const Security &buyTarget) const;

 public:
  virtual void Setup(Position &,
                     TradingLib::PositionController &) const override;

  const OpenOrderPolicy &GetOpenOrderPolicy() const {
    return m_openOrderPolicy;
  }
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const Position &) const override {
    return GetOpenOrderPolicy();
  }
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const Position &) const override {
    return m_closeOrderPolicy;
  }

  virtual bool IsLong(const Security &security) const override {
    Assert(&security == &m_sellTarget || &security == &m_buyTarget);
    return &security == &m_buyTarget;
  }

  virtual trdk::Qty GetPlannedQty(const Security &) const override {
    return m_maxQty;
  }

 private:
  OpenOrderPolicy m_openOrderPolicy;
  CloseOrderPolicy m_closeOrderPolicy;
  Security &m_sellTarget;
  Security &m_buyTarget;
  const Qty m_maxQty;
  const boost::optional<boost::posix_time::time_duration> m_stopLossDelay;
};
}  // namespace ArbitrageAdvisor
}  // namespace Strategies
}  // namespace trdk