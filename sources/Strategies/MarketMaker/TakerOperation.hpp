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
  class PassivePolicy : public TradingLib::LimitGtcOrderPolicy {
   public:
    virtual ~PassivePolicy() override = default;

   public:
    virtual trdk::Price GetOpenOrderPrice(trdk::Position &) const override;
    virtual trdk::Price GetCloseOrderPrice(trdk::Position &) const override;
  };
  class AggresivePolicy : public TradingLib::OrderPolicy {
   public:
    virtual ~AggresivePolicy() override = default;

   public:
    virtual void Open(trdk::Position &) const override;
    virtual void Close(trdk::Position &) const override;

   private:
    static OrderParams m_orderParams;
  };

 public:
  explicit TakerOperation(Strategy &, bool isLong, const Qty &);
  virtual ~TakerOperation() override = default;

 public:
  virtual const trdk::TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const trdk::Position &) const override;
  virtual const trdk::TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const trdk::Position &) const override;

  virtual bool IsLong(const trdk::Security &) const override;

  virtual bool OnCloseReasonChange(trdk::Position &,
                                   const trdk::CloseReason &newReason) override;

  virtual boost::shared_ptr<trdk::Operation> StartInvertedPosition(
      const trdk::Position &) override;

  virtual trdk::Qty GetPlannedQty(const Security &) const override;

 private:
  const bool m_isLong;
  const Qty m_qty;
  AggresivePolicy m_aggresiveOrderPolicy;
  PassivePolicy m_passivePolicy;
};

}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk