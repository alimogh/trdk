/*******************************************************************************
 *   Created: 2017/10/21 17:12:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "StopLimit.hpp"
#include "Core/Position.hpp"
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

namespace pt = boost::posix_time;

TakeProfitStopLimit::TakeProfitStopLimit(
    const boost::shared_ptr<const Params> &params,
    Position &position,
    const boost::shared_ptr<const OrderPolicy> &orderPolicy)
    : StopOrder(position, orderPolicy), m_params(params), m_isActivated(false) {
  Assert(m_params);
}

const char *TakeProfitStopLimit::GetName() const { return "stop limit"; }

void TakeProfitStopLimit::Report(const Position &position,
                                 ModuleTradingLog &log) const {
  log.Write("%1%\tattach\tprice=%2$.8f\ttime=%3%\tclose-vol-ratio=%4%\tpos=%5%",
            [this, &position](TradingRecord &record) {
              record % GetName()                                     // 1
                  % m_params->GetMaxPriceOffsetPerLotToClose()       // 2
                  % m_params->GetTimeOffsetBeforeForcedActivation()  // 3
                  % m_params->GetVolumeToCloseRatio()                // 4
                  % position.GetId();                                // 5
            });
}

void TakeProfitStopLimit::Run() {
  if (m_isActivated || !GetPosition().IsOpened()) {
    return;
  }

  static_assert(numberOfCloseReasons == 13, "List changed.");
  switch (GetPosition().GetCloseReason()) {
    case CLOSE_REASON_NONE:
      if (!CheckSignal()) {
        return;
      }
      GetPosition().SetCloseReason(CLOSE_REASON_STOP_LIMIT);
      m_isActivated = true;
      break;
    default:
      return;
  }

  OnHit();
}

bool TakeProfitStopLimit::CheckSignal() {
  {
    const auto &openPrice = GetPosition().GetOpenAvgPrice();
    const auto &currentPrice = GetPosition().GetMarketClosePrice();
    const auto &diff = m_params->GetMaxPriceOffsetPerLotToClose();
    if (GetPosition().IsLong()) {
      const auto &controlPrice = openPrice + diff;
      if (controlPrice >= currentPrice) {
        GetTradingLog().Write(
            "%1%\tsignaling by price\tprice=(%2$.8f+%3$.8f=%4$.8f)>=%5$.8f"
            "\tbid/ask=%6$.8f/%7$.8f\tpos=%8%",
            [&](TradingRecord &record) {
              record % GetName()                                    // 1
                  % openPrice                                       // 2
                  % diff                                            // 3
                  % controlPrice                                    // 4
                  % currentPrice                                    // 5
                  % GetPosition().GetSecurity().GetBidPriceValue()  // 6
                  % GetPosition().GetSecurity().GetAskPriceValue()  // 7
                  % GetPosition().GetId();                          // 8
            });
        return true;
      }
    } else {
      const auto &controlPrice = openPrice - diff;
      if (controlPrice <= currentPrice) {
        GetTradingLog().Write(
            "%1%\tsignaling by price\tprice=(%2$.8f-%3$.8f=%4$.8f)>=%5$.8f"
            "\tbid/ask=%6$.8f/%7$.8f\tpos=%8%",
            [&](TradingRecord &record) {
              record % GetName()                                    // 1
                  % openPrice                                       // 2
                  % diff                                            // 3
                  % controlPrice                                    // 4
                  % currentPrice                                    // 5
                  % GetPosition().GetSecurity().GetBidPriceValue()  // 6
                  % GetPosition().GetSecurity().GetAskPriceValue()  // 7
                  % GetPosition().GetId();                          // 8
              return true;
            });
      }
    }
  }
  {
    const auto &openTime = GetPosition().GetOpenTime();
    const auto &diff = m_params->GetTimeOffsetBeforeForcedActivation();
    const auto &controlTime = openTime + diff;
    const auto &currentTime =
        GetPosition().GetSecurity().GetContext().GetCurrentTime();
    if (controlTime >= currentTime) {
      GetTradingLog().Write(
          "%1%\tsignaling by time\tprice=(%2%+%3%=%4%)>=%5%\tbid/ask=%6$.8f/"
          "%7$.8f\tpos=%8%",
          [&](TradingRecord &record) {
            record % GetName()                                    // 1
                % openTime.time_of_day()                          // 2
                % diff                                            // 3
                % controlTime.time_of_day()                       // 4
                % currentTime.time_of_day()                       // 5
                % GetPosition().GetSecurity().GetBidPriceValue()  // 6
                % GetPosition().GetSecurity().GetAskPriceValue()  // 7
                % GetPosition().GetId();                          // 8
          });
      return true;
    }
  }
  return false;
}
