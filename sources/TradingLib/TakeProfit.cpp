/**************************************************************************
 *   Created: 2016/12/18 17:13:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TakeProfit.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

////////////////////////////////////////////////////////////////////////////////

TakeProfit::TakeProfit(Position &position, PositionController &controller)
    : StopOrder(position, controller), m_isActivated(false) {}

const Volume &TakeProfit::GetMaxProfit() const {
  Assert(m_maxProfit);
  return *m_maxProfit;
}

void TakeProfit::Run() {
  if (!GetPosition().IsOpened()) {
    return;
  }

  static_assert(numberOfCloseReasons == 13, "List changed.");
  switch (GetPosition().GetCloseReason()) {
    case CLOSE_REASON_NONE:
      if (!CheckSignal()) {
        return;
      }
      break;
    case CLOSE_REASON_TAKE_PROFIT:
      break;
    default:
      return;
  }

  OnHit(CLOSE_REASON_TAKE_PROFIT);
}

bool TakeProfit::CheckSignal() {
  const auto &plannedPnl = GetPosition().GetPlannedPnl();
  if (!Activate(plannedPnl)) {
    Assert(!m_minProfit);
    return false;
  }
  Assert(m_maxProfit);

  auto profitToClose = *m_maxProfit;
  profitToClose -= CalcOffsetToClose();

  if (m_minProfit && plannedPnl >= *m_minProfit) {
    return false;
  }

  const bool isSignal = plannedPnl <= profitToClose;

#if 0
  GetTradingLog().Write(
      "%1%\t%2%"
      "\tprofit=%3$.8f->%4$.8f%5%(%6$.8f-%7$.8f*%8$.8f=%9$.8f)"
      "\tbid/ask=%10$.8f/%11$.8f\tpos=%12%/%13%",
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
        record % *m_maxProfit                                 // 6
            % m_params->GetMaxPriceOffsetPerLotToClose()      // 7
            % GetPosition().GetOpenedQty()                    // 8
            % profitToClose                                   // 9
            % GetPosition().GetSecurity().GetBidPriceValue()  // 10
            % GetPosition().GetSecurity().GetAskPriceValue()  // 11
            % GetPosition().GetOperation()->GetId()           // 12
            % GetPosition().GetSubOperationId();              // 13
      });
#endif

  m_minProfit = plannedPnl;

  return isSignal;
}

bool TakeProfit::Activate(const trdk::Volume &plannedPnl) {
  const auto &profitToActivate = CalcProfitToActivate();

  bool isSignal = false;
  if (!m_isActivated) {
    m_isActivated = plannedPnl >= profitToActivate;
    if (!m_isActivated && m_maxProfit && plannedPnl <= *m_maxProfit) {
      return false;
    }
    isSignal = m_isActivated;
  } else {
    Assert(m_maxProfit);
    if (plannedPnl <= *m_maxProfit) {
      return m_isActivated;
    }
  }

  GetTradingLog().Write(
      "%1%\t%2%"
      "\tprofit=%3$.8f->%4$.8f%5%%6$.8f\tmin-profit=%7$.8f"
      "\tbid/ask=%8$.8f/%9$.8f\tpos=%10%/%11%",
      [&](TradingRecord &record) {
        record % GetName() % (isSignal ? "activating" : "accumulating");
        if (m_maxProfit) {
          record % *m_maxProfit;
        } else {
          record % "none";
        }
        record % plannedPnl;
        if (m_isActivated) {
          record % ">=";
        } else {
          record % '<';
        }
        record % profitToActivate;
        if (m_minProfit) {
          record % *m_minProfit;
        } else {
          record % "none";
        }
        record % GetPosition().GetSecurity().GetBidPriceValue() %
            GetPosition().GetSecurity().GetAskPriceValue() %
            GetPosition().GetOperation()->GetId()  // 10
            % GetPosition().GetSubOperationId();   // 11
      });

  m_maxProfit = plannedPnl;
  m_minProfit.reset();

  return m_isActivated;
}

////////////////////////////////////////////////////////////////////////////////

TakeProfitPrice::Params::Params(const Volume &profitPerLotToActivate,
                                const Volume &trailingToClose)
    : m_profitPerLotToActivate(profitPerLotToActivate),
      m_trailingToClose(trailingToClose) {}
const Volume &TakeProfitPrice::Params::GetProfitPerLotToActivate() const {
  return m_profitPerLotToActivate;
}
const Volume &TakeProfitPrice::Params::GetTrailingToClose() const {
  return m_trailingToClose;
}

TakeProfitPrice::TakeProfitPrice(const boost::shared_ptr<const Params> &params,
                                 Position &position,
                                 PositionController &controller)
    : TakeProfit(position, controller), m_params(params) {
  Assert(m_params);
}

void TakeProfitPrice::Report(const char *action) const {
  GetTradingLog().Write(
      "{'algo': {'action': '%1%', 'type': '%2%', 'params': {'profit': %3$.8f, "
      "'trailing': %4$.8f}, 'operation': '%5%/%6%'}}",
      [this, action](TradingRecord &record) {
        record % action                              // 1
            % GetName()                              // 2
            % m_params->GetProfitPerLotToActivate()  // 3
            % m_params->GetTrailingToClose()         // 4
            % GetPosition().GetOperation()->GetId()  // 5
            % GetPosition().GetSubOperationId();     // 6
      });
}

const char *TakeProfitPrice::GetName() const { return "take profit price"; }

Volume TakeProfitPrice::CalcProfitToActivate() const {
  return m_params->GetProfitPerLotToActivate() * GetPosition().GetOpenedQty();
}

Volume TakeProfitPrice::CalcOffsetToClose() const {
  return m_params->GetTrailingToClose() * GetPosition().GetOpenedQty();
}

////////////////////////////////////////////////////////////////////////////////

TakeProfitShare::Params::Params(const Volume &profitShareToActivate,
                                const Volume &trailingShareToClose)
    : m_profitShareToActivate(profitShareToActivate),
      m_trailingShareToClose(trailingShareToClose) {}
const Volume &TakeProfitShare::Params::GetProfitShareToActivate() const {
  return m_profitShareToActivate;
}
const Volume &TakeProfitShare::Params::GetTrailingShareToClose() const {
  return m_trailingShareToClose;
}

TakeProfitShare::TakeProfitShare(const boost::shared_ptr<const Params> &params,
                                 Position &position,
                                 PositionController &controller)
    : TakeProfit(position, controller), m_params(params) {
  Assert(m_params);
}

void TakeProfitShare::Report(const char *action) const {
  GetTradingLog().Write(
      "{'algo': {'action': '%1%', 'type': '%2%', 'params': {'profit': %3$.8f, "
      "'trailing': %4$.8f}, 'operation': '%5%/%6%'}}",
      [this, action](TradingRecord &record) {
        record % action                              // 1
            % GetName()                              // 2
            % m_params->GetProfitShareToActivate()   // 3
            % m_params->GetTrailingShareToClose()    // 4
            % GetPosition().GetOperation()->GetId()  // 5
            % GetPosition().GetSubOperationId();     // 6
      });
}

const char *TakeProfitShare::GetName() const { return "take profit share"; }

Volume TakeProfitShare::CalcProfitToActivate() const {
  const auto &initialVol = GetPosition().GetOpenedVolume();
  return initialVol + (initialVol * m_params->GetProfitShareToActivate());
}

Volume TakeProfitShare::CalcOffsetToClose() const {
  return GetMaxProfit() * m_params->GetTrailingShareToClose();
}

////////////////////////////////////////////////////////////////////////////////
