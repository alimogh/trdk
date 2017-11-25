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

////////////////////////////////////////////////////////////////////////////////

class Operation : public trdk::Operation {
 public:
  typedef trdk::Operation Base;

 public:
  explicit Operation(Security &sellTarget,
                     Security &buyTarget,
                     const Qty &maxQty,
                     const Price &sellPrice,
                     const Price &buyPrice)
      : m_orderPolicy(sellPrice, buyPrice),
        m_sellTarget(sellTarget),
        m_buyTarget(buyTarget),
        m_maxQty(maxQty) {}
  virtual ~Operation() override = default;

 public:
  bool IsSame(const Security &sellTarget, const Security &buyTarget) const {
    return &m_sellTarget == &sellTarget && &m_buyTarget == &buyTarget;
  }

  BusinessOperationReportData &GetReportData() { return m_reportData; }
  const BusinessOperationReportData &GetReportData() const {
    return const_cast<Operation *>(this)->GetReportData();
  }

 public:
  virtual const trdk::TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const Position &) const override {
    return m_orderPolicy;
  }

  virtual bool IsLong(const Security &security) const override {
    Assert(&security == &m_sellTarget || &security == &m_buyTarget);
    return &security == &m_buyTarget;
  }

  virtual trdk::Qty GetPlannedQty() const override { return m_maxQty; }

 private:
  OrderPolicy m_orderPolicy;
  Security &m_sellTarget;
  Security &m_buyTarget;
  const Qty m_maxQty;
  BusinessOperationReportData m_reportData;
};

////////////////////////////////////////////////////////////////////////////////

class BalanceRestoringOperation : public trdk::Operation {
 public:
  typedef trdk::Operation Base;

 public:
  explicit BalanceRestoringOperation(const boost::shared_ptr<Operation> &parent,
                                     const Price &price)
      : m_parent(parent), m_orderPolicy(price) {}
  virtual ~BalanceRestoringOperation() override = default;

 public:
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const Position &) const override {
    return m_orderPolicy;
  }
  virtual boost::shared_ptr<const Operation> GetParent() const override {
    return m_parent;
  }
  virtual boost::shared_ptr<Operation> GetParent() override { return m_parent; }

  virtual bool HasCloseSignal(const Position &) const { return true; }

  virtual boost::shared_ptr<Operation> StartInvertedPosition(const Position &) {
    return nullptr;
  }

 private:
  const boost::shared_ptr<Operation> m_parent;
  const TradingLib::LimitPriceGtcOrderPolicy m_orderPolicy;
};

////////////////////////////////////////////////////////////////////////////////
}
}
}