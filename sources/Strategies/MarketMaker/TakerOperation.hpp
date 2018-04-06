/*******************************************************************************
 *   Created: 2018/02/21 15:50:05
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

class TakerOperation : public Operation {
 public:
  typedef Operation Base;

 private:
  class PnlContainer : public TradingLib::PnlOneSymbolContainer {
   public:
    typedef PnlOneSymbolContainer Base;

    ~PnlContainer() override = default;

    Result GetResult() const override;
  };

  class OrderPolicy : public TradingLib::LimitGtcOrderPolicy {
   public:
    ~OrderPolicy() override = default;

    Price GetOpenOrderPrice(Position &) const override;
    Price GetCloseOrderPrice(Position &) const override;
  };

 public:
  explicit TakerOperation(Strategy &, bool isLong, const Qty &);
  ~TakerOperation() override = default;

  const TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const Position &) const override;
  const TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const Position &) const override;

  bool IsLong(const Security &) const override;

  bool OnCloseReasonChange(Position &, const CloseReason &) override;

  boost::shared_ptr<Operation> StartInvertedPosition(const Position &) override;

  Qty GetPlannedQty(const Security &) const override;

  bool HasCloseSignal(const Position &) const override;

 private:
  const bool m_isLong;
  const Qty m_qty;
  OrderPolicy m_orderPolicy;
};

}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk