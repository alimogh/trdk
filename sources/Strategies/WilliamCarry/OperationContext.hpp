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

namespace trdk {
namespace Strategies {
namespace WilliamCarry {

class OperationContext : public trdk::PositionOperationContext {
 public:
  typedef trdk::PositionOperationContext Base;

 public:
  explicit OperationContext(const Qty &,
                            const Price &price,
                            TradingSystem &tradingSystem);
  OperationContext(OperationContext &&);
  virtual ~OperationContext() override;

 public:
  void AddTakeProfitStopLimit(
      const Price &maxPriceChange,
      const boost::posix_time::time_duration &activationTime);
  void AddStopLoss(const Price &maxPriceChange);
  void AddStopLoss(const Price &maxPriceChange,
                   const boost::posix_time::time_duration &startDelay);

  bool HasSubOperations() const;
  std::pair<std::vector<boost::shared_ptr<PositionOperationContext>>,
            std::vector<boost::shared_ptr<PositionOperationContext>>>
  StartSubOperations(Position &rootOperation);

 public:
  virtual trdk::TradingSystem &GetTradingSystem(trdk::Strategy &,
                                                trdk::Security &) override;
  virtual const trdk::TradingSystem &GetTradingSystem(
      const trdk::Strategy &, const trdk::Security &) const override;
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const Position &) const override;
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const Position &) const override;
  virtual void Setup(Position &) const override;
  virtual bool IsLong(const trdk::Security &) const override;
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