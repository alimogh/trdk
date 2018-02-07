/**************************************************************************
 *   Created: 2016/12/18 17:10:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "StopOrder.hpp"

namespace trdk {
namespace TradingLib {

////////////////////////////////////////////////////////////////////////////////

class TakeProfit : public trdk::TradingLib::StopOrder {
 public:
  explicit TakeProfit(trdk::Position &, trdk::TradingLib::PositionController &);
  virtual ~TakeProfit() override = default;

 public:
  virtual void Run() override;

 protected:
  const trdk::Volume &GetMaxProfit() const;

  virtual trdk::Volume CalcProfitToActivate() const = 0;
  virtual trdk::Volume CalcOffsetToClose() const = 0;

 private:
  bool CheckSignal();
  bool Activate(const trdk::Volume &plannedPnl);

 private:
  bool m_isActivated;
  boost::optional<trdk::Volume> m_minProfit;
  boost::optional<trdk::Volume> m_maxProfit;
};

////////////////////////////////////////////////////////////////////////////////

class TakeProfitPrice : public trdk::TradingLib::TakeProfit {
 public:
  class Params {
   public:
    explicit Params(const trdk::Volume &profitPerLotToActivate,
                    const trdk::Volume &trailingToClose);

   public:
    const trdk::Volume &GetProfitPerLotToActivate() const;
    const trdk::Volume &GetTrailingToClose() const;

   private:
    trdk::Volume m_profitPerLotToActivate;
    trdk::Volume m_trailingToClose;
  };

 public:
  explicit TakeProfitPrice(const boost::shared_ptr<const Params> &,
                           trdk::Position &,
                           trdk::TradingLib::PositionController &);
  virtual ~TakeProfitPrice() override = default;

 public:
  virtual void Report(const char *action) const override;

 protected:
  virtual const char *GetName() const override;
  virtual trdk::Volume CalcProfitToActivate() const override;
  virtual trdk::Volume CalcOffsetToClose() const override;

 private:
  const boost::shared_ptr<const Params> m_params;
};

////////////////////////////////////////////////////////////////////////////////

class TakeProfitShare : public trdk::TradingLib::TakeProfit {
 public:
  class Params {
   public:
    explicit Params(const trdk::Volume &profitShareToActivate,
                    const trdk::Volume &trailingShareToClose);

   public:
    const trdk::Volume &GetProfitShareToActivate() const;
    const trdk::Volume &GetTrailingShareToClose() const;

   private:
    trdk::Volume m_profitShareToActivate;
    trdk::Volume m_trailingShareToClose;
  };

 public:
  explicit TakeProfitShare(const boost::shared_ptr<const Params> &,
                           trdk::Position &,
                           trdk::TradingLib::PositionController &);
  virtual ~TakeProfitShare() override = default;

 public:
  virtual void Report(const char *action) const override;

 protected:
  virtual const char *GetName() const override;
  virtual trdk::Volume CalcProfitToActivate() const override;
  virtual trdk::Volume CalcOffsetToClose() const override;

 private:
  const boost::shared_ptr<const Params> m_params;
};

////////////////////////////////////////////////////////////////////////////////

}  // namespace TradingLib
}  // namespace trdk
