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

class TakeProfit : public trdk::TradingLib::StopOrder {
 public:
  class Params {
   public:
    explicit Params(const trdk::Volume &minProfitPerLotToActivate,
                    const trdk::Volume &maxPriceOffsetPerLotToClose);

   public:
    const trdk::Volume &GetMinProfitPerLotToActivate() const;
    const trdk::Volume &GetMaxPriceOffsetPerLotToClose() const;

   private:
    trdk::Volume m_minProfitPerLotToActivate;
    trdk::Volume m_maxPriceOffsetPerLotToClose;
  };

 public:
  explicit TakeProfit(
      const boost::shared_ptr<const Params> &,
      trdk::Position &,
      const boost::shared_ptr<const trdk::TradingLib::OrderPolicy> &);
  virtual ~TakeProfit();

 public:
  virtual void Run() override;

 protected:
  virtual const char *GetName() const override;

 private:
  bool CheckSignal();
  bool Activate(const trdk::Volume &plannedPnl);

 private:
  const boost::shared_ptr<const Params> m_params;

  bool m_isActivated;
  boost::optional<trdk::Lib::Double> m_minProfit;
  boost::optional<trdk::Lib::Double> m_maxProfit;
};
}
}
