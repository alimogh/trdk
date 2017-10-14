/*******************************************************************************
 *   Created: 2017/10/14 00:09:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/PositionOperationContext.hpp"
#include "WilliamCarryApi.h"

namespace trdk {
namespace Strategies {
namespace WilliamCarry {

class TRDK_STRATEGY_WILLIAMCARRY_API OperationContext
    : public trdk::PositionOperationContext {
 public:
  typedef trdk::PositionOperationContext Base;

 public:
  explicit OperationContext(bool isLong, const Qty &);
  OperationContext(OperationContext &&);
  virtual ~OperationContext() override;

 public:
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy() const override;
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy() const override;
  virtual void Setup(Position &) const override;
  virtual bool IsLong() const override;
  virtual Qty GetPlannedQty() const override;
  virtual bool HasCloseSignal(const Position &) const override;
  virtual bool IsInvertible(const Position &) const override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}