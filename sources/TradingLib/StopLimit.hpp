/*******************************************************************************
 *   Created: 2017/10/21 16:28:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "StopOrder.hpp"

namespace trdk {
namespace TradingLib {

class TakeProfitStopLimit : public trdk::TradingLib::StopOrder {
 public:
  class Params {
   public:
    explicit Params(const trdk::Volume &maxPriceOffsetPerLotToClose,
                    const boost::posix_time::time_duration
                        &timeOffsetBeforeForcedActivation)
        : m_maxPriceOffsetPerLotToClose(maxPriceOffsetPerLotToClose),
          m_timeOffsetBeforeForcedActivation(timeOffsetBeforeForcedActivation) {
    }

   public:
    const trdk::Volume &GetMaxPriceOffsetPerLotToClose() const {
      return m_maxPriceOffsetPerLotToClose;
    }
    const boost::posix_time::time_duration &
    GetTimeOffsetBeforeForcedActivation() const {
      return m_timeOffsetBeforeForcedActivation;
    }

   private:
    trdk::Volume m_maxPriceOffsetPerLotToClose;
    boost::posix_time::time_duration m_timeOffsetBeforeForcedActivation;
  };

 public:
  explicit TakeProfitStopLimit(
      const boost::shared_ptr<const Params> &,
      trdk::Position &,
      const boost::shared_ptr<const trdk::TradingLib::OrderPolicy> &);
  virtual ~TakeProfitStopLimit() override = default;

 public:
  virtual void Run() override;

  virtual void Report(const trdk::Position &,
                      trdk::ModuleTradingLog &) const override;

 protected:
  virtual const char *GetName() const override;

 private:
  bool CheckSignal();

 private:
  const boost::shared_ptr<const Params> m_params;
  bool m_isActivated;
};
}
}
