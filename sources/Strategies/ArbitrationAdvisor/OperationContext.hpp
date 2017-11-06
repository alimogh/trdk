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

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {
class OperationContext : public PositionOperationContext {
 public:
  explicit OperationContext(Security &sellTarget,
                            Security &buyTarget,
                            const Qty &maxQty)
      : m_sellTarget(sellTarget), m_buyTarget(buyTarget), m_maxQty(maxQty) {}
  virtual ~OperationContext() override = default;

 public:
  virtual const trdk::TradingLib::OrderPolicy &GetOpenOrderPolicy()
      const override {
    return m_orderPolicy;
  }
  virtual const trdk::TradingLib::OrderPolicy &GetCloseOrderPolicy()
      const override {
    return m_orderPolicy;
  }

  virtual void Setup(trdk::Position &) const override {}

  virtual bool IsLong(const Security &security) const override {
    Assert(&security == &m_sellTarget || &security == &m_buyTarget);
    return &security == &m_buyTarget;
  }

  virtual trdk::Qty GetPlannedQty() const override { return m_maxQty; }

  virtual bool HasCloseSignal(const trdk::Position &) const override {
    // Not used in this strategy.
    return false;
  }

  virtual boost::shared_ptr<PositionOperationContext> StartInvertedPosition(
      const trdk::Position &) override {
    return nullptr;
  }

 private:
  TradingLib::LimitGtcOrderPolicy m_orderPolicy;
  Security &m_sellTarget;
  Security &m_buyTarget;
  const Qty m_maxQty;
};
}
}
}