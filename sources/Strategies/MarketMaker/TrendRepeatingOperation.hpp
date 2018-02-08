/*******************************************************************************
 *   Created: 2018/02/05 00:22:45
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
namespace MarketMaker {

class TrendRepeatingOperation : public Operation {
 public:
  typedef Operation Base;

 public:
  explicit TrendRepeatingOperation(
      Strategy &,
      const Qty &,
      bool isLong,
      const boost::shared_ptr<TradingLib::TakeProfitShare::Params> &,
      const boost::shared_ptr<TradingLib::StopLossShare::Params> &);
  virtual ~TrendRepeatingOperation() override = default;

 public:
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const Position &) const override;
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const Position &) const override;
  virtual void Setup(Position &,
                     TradingLib::PositionController &) const override;

  virtual bool IsLong(const Security &) const override;
  virtual Qty GetPlannedQty() const override;
  virtual bool HasCloseSignal(const Position &) const override;
  virtual boost::shared_ptr<Operation> StartInvertedPosition(
      const Position &) override;

 private:
  TradingLib::LimitIocOrderPolicy m_orderPolicy;
  const Qty m_qty;
  const bool m_isLong;
  const boost::shared_ptr<const TradingLib::TakeProfitShare::Params>
      m_takeProfit;
  const boost::shared_ptr<const TradingLib::StopLossShare::Params> m_stopLoss;
};

}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk