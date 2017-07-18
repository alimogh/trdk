/**************************************************************************
 *   Created: 2016/12/15 04:15:18
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

class TrailingStop : public trdk::TradingLib::StopOrder {
 public:
  class Params {
   public:
    explicit Params(const trdk::Volume &minProfitPerLotToActivate,
                    const trdk::Volume &minProfitPerLotToClose);

   public:
    const trdk::Volume &GetMinProfitPerLotToActivate() const;
    const trdk::Volume &GetMinProfitPerLotToClose() const;

   private:
    trdk::Volume m_minProfitPerLotToActivate;
    trdk::Volume m_minProfitPerLotToClose;
  };

 public:
  explicit TrailingStop(const boost::shared_ptr<const Params> &,
                        trdk::Position &);
  virtual ~TrailingStop();

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
  boost::optional<trdk::Lib::Double> m_maxProfit;
  boost::optional<trdk::Lib::Double> m_minProfit;
};
}
}
