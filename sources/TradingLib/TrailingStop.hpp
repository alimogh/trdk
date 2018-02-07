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

////////////////////////////////////////////////////////////////////////////////

class TrailingStop : public trdk::TradingLib::StopOrder {
 public:
  explicit TrailingStop(trdk::Position &,
                        trdk::TradingLib::PositionController &);
  virtual ~TrailingStop() override = default;

 public:
  virtual void Run() override;

 protected:
  virtual trdk::Lib::Double CalcProfitToActivate() const = 0;
  virtual trdk::Lib::Double CalcProfitToClose() const = 0;

 private:
  bool CheckSignal();
  bool Activate(const trdk::Volume &plannedPnl);

 private:
  bool m_isActivated;
  boost::optional<trdk::Lib::Double> m_maxProfit;
  boost::optional<trdk::Lib::Double> m_minProfit;
};

////////////////////////////////////////////////////////////////////////////////

class TrailingStopPrice : public trdk::TradingLib::TrailingStop {
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
  explicit TrailingStopPrice(const boost::shared_ptr<const Params> &,
                             trdk::Position &,
                             trdk::TradingLib::PositionController &);
  virtual ~TrailingStopPrice() override = default;

 protected:
  virtual const char *GetName() const override;
  virtual trdk::Lib::Double CalcProfitToActivate() const override;
  virtual trdk::Lib::Double CalcProfitToClose() const override;

 private:
  const boost::shared_ptr<const Params> m_params;
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace TradingLib
}  // namespace trdk
