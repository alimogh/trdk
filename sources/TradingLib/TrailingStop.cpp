/**************************************************************************
 *   Created: 2016/12/15 04:18:41
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TrailingStop.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

////////////////////////////////////////////////////////////////////////////////

TrailingStop::TrailingStop(Position &position, PositionController &controller)
    : StopOrder(position, controller), m_isActivated(false) {}

void TrailingStop::Run() {
  if (!GetPosition().IsOpened()) {
    Assert(!m_isActivated);
    return;
  }

  static_assert(numberOfCloseReasons == 13, "List changed.");
  switch (GetPosition().GetCloseReason()) {
    case CLOSE_REASON_STOP_LOSS:
      return;
    case CLOSE_REASON_TRAILING_STOP:
      Assert(m_isActivated);
      break;
    default:
      if (!CheckSignal()) {
        return;
      }
      break;
  }

  OnHit(CLOSE_REASON_TRAILING_STOP);
}

bool TrailingStop::CheckSignal() {
  AssertNe(CLOSE_REASON_TRAILING_STOP, GetPosition().GetCloseReason());

  const auto &plannedPnl = GetPosition().GetPlannedPnl();
  if (!Activate(plannedPnl)) {
    Assert(!m_minProfit);
    return false;
  }

  const auto &profitToClose = CalcProfitToClose();

  if (m_minProfit && plannedPnl >= *m_minProfit) {
    return false;
  }

  const bool isSignal = plannedPnl <= profitToClose;

#if 0
  GetTradingLog().Write(
      "%1%\t%2%"
      "\tprofit=%3%->%4%%5%(%6%*%7%=%8%)"
      "\tbid/ask=%9%/%10%\tpos=%11%/%12%",
      [&](TradingRecord &record) {
        record % GetName()                            // 1
            % (isSignal ? "signaling" : "trailing");  // 2
        if (m_minProfit) {
          record % *m_minProfit;  // 3
        } else {
          record % "none";  // 3
        }
        record % plannedPnl;  // 4
        if (isSignal) {
          record % "<=";  // 5
        } else {
          record % '>';  // 5
        }
        record % m_params->GetMinProfitPerLotToActivate()     // 6
            % GetPosition().GetOpenedQty()                    // 7
            % profitToClose                                   // 8
            % GetPosition().GetSecurity().GetBidPriceValue()  // 9
            % GetPosition().GetSecurity().GetAskPriceValue()  // 10
            % GetPosition().GetOperation()->GetId()           // 11
            % GetPosition().GetSubOperationId();              // 12
      });
#endif

  m_minProfit = plannedPnl;

  return isSignal;
}

bool TrailingStop::Activate(const trdk::Volume &plannedPnl) {
  if (m_isActivated) {
    return true;
  }

  const auto &profitToActivate = CalcProfitToActivate();

  m_isActivated = plannedPnl >= profitToActivate;

  if (m_maxProfit && plannedPnl <= *m_maxProfit) {
    return m_isActivated;
  }

#if 0
  GetTradingLog().Write(
      "%1%\t%2%"
      "\tprofit=%3%->%4%%5%(%6%*%7%=%8%)"
      "\tbid/ask=%9%/%10%\tpos=%11%/%12%",
      [&](TradingRecord &record) {
        record % GetName()                                      // 1
            % (m_isActivated ? "activating" : "accumulating");  // 2
        if (m_maxProfit) {
          record % *m_maxProfit;  // 3
        } else {
          record % "none";  // 3
        }
        record % plannedPnl;  // 4
        if (m_isActivated) {
          record % ">=";  // 5
        } else {
          record % '<';  // 5
        }
        record % m_params->GetMinProfitPerLotToActivate()     // 6
            % GetPosition().GetOpenedQty()                    // 7
            % profitToActivate                                // 8
            % GetPosition().GetSecurity().GetBidPriceValue()  // 9
            % GetPosition().GetSecurity().GetAskPriceValue()  // 10
            % GetPosition().GetOperation()->GetId()           // 11
            % GetPosition().GetSubOperationId();              // 12
      });
#endif

  m_maxProfit = plannedPnl;

  return m_isActivated;
}

////////////////////////////////////////////////////////////////////////////////

TrailingStopPrice::Params::Params(const Volume &minProfitPerLotToActivate,
                                  const Volume &minProfitPerLotToClose)
    : m_minProfitPerLotToActivate(minProfitPerLotToActivate),
      m_minProfitPerLotToClose(minProfitPerLotToClose) {
  if (m_minProfitPerLotToActivate < m_minProfitPerLotToClose) {
    throw Exception(
        "Min profit to activate trailing stop must be greater than"
        " min profit to close position by trailing stop");
  }
}

const Volume &TrailingStopPrice::Params::GetMinProfitPerLotToActivate() const {
  return m_minProfitPerLotToActivate;
}

const Volume &TrailingStopPrice::Params::GetMinProfitPerLotToClose() const {
  return m_minProfitPerLotToClose;
}

TrailingStopPrice::TrailingStopPrice(
    const boost::shared_ptr<const Params> &params,
    Position &position,
    PositionController &controller)
    : TrailingStop(position, controller), m_params(params) {}

const char *TrailingStopPrice::GetName() const { return "trailing stop price"; }

Double TrailingStopPrice::CalcProfitToActivate() const {
  return m_params->GetMinProfitPerLotToActivate() *
         GetPosition().GetOpenedQty();
}

Double TrailingStopPrice::CalcProfitToClose() const {
  return m_params->GetMinProfitPerLotToClose() * GetPosition().GetOpenedQty();
}

////////////////////////////////////////////////////////////////////////////////
