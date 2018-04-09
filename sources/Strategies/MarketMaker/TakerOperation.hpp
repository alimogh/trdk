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
  class AggresivePolicy : public TradingLib::LimitOrderPolicy {
   public:
    ~AggresivePolicy() override = default;

    void Open(Position &) const override;
    void Close(Position &) const override;

   private:
    static OrderParams m_orderParams;
  };

 public:
  explicit TakerOperation(Strategy &);
  ~TakerOperation() override = default;

  const TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const Position &) const override;
  const TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const Position &) const override;

  bool OnCloseReasonChange(Position &, const CloseReason &) override;

  boost::shared_ptr<Operation> StartInvertedPosition(const Position &) override;

  bool HasCloseSignal(const Position &) const override;

 private:
  AggresivePolicy m_aggresiveOrderPolicy;
  TradingLib::LimitGtcOrderPolicy m_passivePolicy;
};

}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk