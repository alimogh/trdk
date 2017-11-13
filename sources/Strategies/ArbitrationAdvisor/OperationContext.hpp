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
#include "Report.hpp"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {
class OperationContext : public PositionOperationContext {
 public:
  explicit OperationContext(Security &sellTarget,
                            Security &buyTarget,
                            const Qty &maxQty,
                            const Price &sellPrice,
                            const Price &buyPrice)
      : m_orderPolicy(sellPrice, buyPrice),
        m_sellTarget(sellTarget),
        m_buyTarget(buyTarget),
        m_maxQty(maxQty) {}
  virtual ~OperationContext() override = default;

 public:
  bool IsSame(const Security &sellTarget, const Security &buyTarget) const {
    return &m_sellTarget == &sellTarget && &m_buyTarget == &buyTarget;
  }

  OperationReportData &GetReportData() { return m_reportData; }
  const OperationReportData &GetReportData() const {
    return const_cast<OperationContext *>(this)->GetReportData();
  }

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
  OrderPolicy m_orderPolicy;
  Security &m_sellTarget;
  Security &m_buyTarget;
  const Qty m_maxQty;
  OperationReportData m_reportData;
};
}
}
}