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
#include "Api.h"

namespace trdk {
namespace Strategies {
namespace WilliamCarry {

class TRDK_STRATEGY_WILLIAMCARRY_API OperationContext
    : public trdk::PositionOperationContext {
 public:
  typedef trdk::PositionOperationContext Base;

 public:
  explicit OperationContext(bool isLong, const Qty &, const Price &price);
  OperationContext(OperationContext &&);
  virtual ~OperationContext() override;

 public:
  void AddTakeProfitStopLimit(
      const Price &maxPriceChange,
      const boost::posix_time::time_duration &activationTime,
      const Lib::Double &volumeToCloseRatio);
  void AddStopLoss(const Price &maxPriceChange);
  void AddStopLoss(const Price &maxPriceChange,
                   const boost::posix_time::time_duration &startDelay);

 public:
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy() const override;
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy() const override;
  virtual void Setup(Position &) const override;
  virtual bool IsLong() const override;
  virtual Qty GetPlannedQty() const override;
  virtual bool HasCloseSignal(const Position &) const override;
  virtual boost::shared_ptr<PositionOperationContext> StartInvertedPosition(
      const trdk::Position &) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}