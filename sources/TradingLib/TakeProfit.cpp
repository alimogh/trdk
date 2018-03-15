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
  const auto offset = CalcOffsetToClose();
  profitToClose -= offset;

  if (m_minProfit && plannedPnl >= *m_minProfit) {
    return false;
  }

  const bool isSignal = plannedPnl <= profitToClose;

  GetTradingLog().Write(
      "{'algo': {'action': '%1%', 'type': '%2%', 'plannedPnl': %3%, 'signal': "
      "{'min': %4%, 'max': %5%, 'offset': %6%, 'toClose': %7%}, 'bid': %8%, "
      "'ask': %9%, 'position': {'operation': '%10%/%11%'}}}",
      [&](TradingRecord &record) {
        record % GetName()                           // 1
            % (isSignal ? "signaling" : "trailing")  // 2
            % plannedPnl;                            // 3
        if (m_minProfit) {
          record % *m_minProfit;  // 4
        } else {
          record % "null";  // 4
        }
        if (m_maxProfit) {
          record % *m_maxProfit;  // 5
        } else {
          record % "null";  // 5
        }
        record % offset                                       // 6
            % profitToClose                                   // 7
            % GetPosition().GetSecurity().GetBidPriceValue()  // 8
            % GetPosition().GetSecurity().GetAskPriceValue()  // 9
            % GetPosition().GetOperation()->GetId()           // 10
            % GetPosition().GetSubOperationId();              // 11
      });

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
      "{'algo': {'action': '%1%', 'type': '%2%', 'profit': '%3%->%4%%5%%6%', "
      "'minProfit': %7%, 'bid': %8%, 'ask': %9%, 'position': {'operation': "
      "'%10%/%11%'}}}",
      [&](TradingRecord &record) {
        record % GetName()                                 // 1
            % (isSignal ? "activating" : "accumulating");  // 2
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
        record % profitToActivate;  // 6
        if (m_minProfit) {
          record % *m_minProfit;  // 7
        } else {
          record % "null";  // 7
        }
        record % GetPosition().GetSecurity().GetBidPriceValue()  // 8
            % GetPosition().GetSecurity().GetAskPriceValue()     // 9
            % GetPosition().GetOperation()->GetId()              // 10
            % GetPosition().GetSubOperationId();                 // 11
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
      "{'algo': {'action': '%1%', 'type': '%2%', 'params': {'profit': %3%, "
      "'trailing': %4%}, 'operation': '%5%/%6%'}}",
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
      "{'algo': {'action': '%1%', 'type': '%2%', 'params': {'profit': %3%, "
      "'trailing': %4%}, 'operation': '%5%/%6%'}}",
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
  return initialVol * m_params->GetProfitShareToActivate();
}

Volume TakeProfitShare::CalcOffsetToClose() const {
  return GetMaxProfit() * m_params->GetTrailingShareToClose();
}

////////////////////////////////////////////////////////////////////////////////
