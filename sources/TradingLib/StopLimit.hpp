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

class TakeProfitStopLimit : public StopOrder {
 public:
  class Params {
   public:
    explicit Params(const Volume &maxPriceOffsetPerLotToClose,
                    const boost::posix_time::time_duration
                        &timeOffsetBeforeForcedActivation)
        : m_maxPriceOffsetPerLotToClose(maxPriceOffsetPerLotToClose),
          m_timeOffsetBeforeForcedActivation(timeOffsetBeforeForcedActivation) {
    }

    const Volume &GetMaxPriceOffsetPerLotToClose() const {
      return m_maxPriceOffsetPerLotToClose;
    }
    const boost::posix_time::time_duration &
    GetTimeOffsetBeforeForcedActivation() const {
      return m_timeOffsetBeforeForcedActivation;
    }

   private:
    Volume m_maxPriceOffsetPerLotToClose;
    boost::posix_time::time_duration m_timeOffsetBeforeForcedActivation;
  };

  explicit TakeProfitStopLimit(const boost::shared_ptr<const Params> &,
                               Position &,
                               PositionController &);
  ~TakeProfitStopLimit() override = default;

  void Run() override;

  void Report(const char *action) const override;

 protected:
  const char *GetName() const override;

 private:
  bool CheckSignal();

  const boost::shared_ptr<const Params> m_params;
  bool m_isActivated;
};
}  // namespace TradingLib
}  // namespace trdk
