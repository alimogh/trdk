/**************************************************************************
 *   Created: 2016/12/14 03:07:18
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "StopLoss.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

StopLossOrder::StopLossOrder(Position &position, PositionController &controller)
    : StopOrder(position, controller), m_delay(pt::not_a_date_time) {}

StopLossOrder::StopLossOrder(const pt::time_duration &delay,
                             Position &position,
                             PositionController &controller)
    : StopOrder(position, controller), m_delay(delay) {}

bool StopLossOrder::IsWatching() const { return GetPosition().IsOpened(); }

const pt::ptime &StopLossOrder::GetStartTime() const {
  return GetPosition().GetOpenTime();
}

void StopLossOrder::Run() {
  if (!IsWatching() ||
      (m_delay != pt::not_a_date_time &&
       GetStartTime() + m_delay >
           GetPosition().GetSecurity().GetContext().GetCurrentTime())) {
    return;
  }

  static_assert(numberOfCloseReasons == 13, "List changed.");
  switch (GetPosition().GetCloseReason()) {
    case CLOSE_REASON_STOP_LOSS:
      break;

    case CLOSE_REASON_TRAILING_STOP:
      return;

    default:
      if (!Activate()) {
        return;
      }
      break;
  }

  OnHit(CLOSE_REASON_STOP_LOSS);
}

////////////////////////////////////////////////////////////////////////////////

StopPrice::Params::Params(const Price &price) : m_price(price) {}

const Price &StopPrice::Params::GetPrice() const { return m_price; }

StopPrice::StopPrice(const boost::shared_ptr<const Params> &params,
                     Position &position,
                     PositionController &controller)
    : StopLossOrder(position, controller), m_params(params) {}

void StopPrice::Report(const char *action) const {
  GetTradingLog().Write(
      "{'algo': {'action': '%6%', 'type': '%1%', 'params': {'price': "
      "'%7% %2$.8f'}, 'delayTime': '%3%', 'position': {'side': '%8%', "
      "'operation': '%4%/%5%'}}}",
      [this, action](TradingRecord &record) {
        record % GetName()                            // 1
            % m_params->GetPrice()                    // 2
            % GetDelay()                              // 3
            % GetPosition().GetOperation()->GetId()   // 4
            % GetPosition().GetSubOperationId()       // 5
            % action                                  // 6
            % (GetPosition().IsLong() ? "<=" : ">=")  // 7
            % GetPosition().GetSide();                // 8
      });
}

bool StopPrice::Activate() {
  const auto &currentPrice = GetActualPrice();
  if (GetPosition().IsLong()) {
    if (m_params->GetPrice() < currentPrice) {
      return false;
    }
  } else if (m_params->GetPrice() > currentPrice) {
    return false;
  }

  GetTradingLog().Write(
      "{'algo': {'action': 'hit', 'type': '%1%', 'price': '%2$.8f%3%%4$.8f', "
      "'bid': %5$.8f, 'ask': %6$.8f, 'position': {'side': '%9%', 'operation': "
      "'%7%/%8%'}}}",
      [&](TradingRecord &record) {
        record % GetName()                                    // 1
            % currentPrice                                    // 2
            % (GetPosition().IsLong() ? "<=" : ">=")          // 3
            % m_params->GetPrice()                            // 4
            % GetPosition().GetSecurity().GetBidPriceValue()  // 5
            % GetPosition().GetSecurity().GetAskPriceValue()  // 6
            % GetPosition().GetOperation()->GetId()           // 7
            % GetPosition().GetSubOperationId()               // 8
            % GetPosition().GetSide();                        // 9
      });

  return true;
}

StopLastPrice::StopLastPrice(const boost::shared_ptr<const Params> &params,
                             Position &position,
                             PositionController &controller)
    : StopPrice(params, position, controller) {}

const char *StopLastPrice::GetName() const { return "stop last price"; }

Price StopLastPrice::GetActualPrice() const {
  return GetPosition().GetSecurity().GetLastPrice();
}

StopBidAskPrice::StopBidAskPrice(const boost::shared_ptr<const Params> &params,
                                 Position &position,
                                 PositionController &controller)
    : StopPrice(params, position, controller) {}

const char *StopBidAskPrice::GetName() const { return "stop bid-ask price"; }

Price StopBidAskPrice::GetActualPrice() const {
  return GetPosition().GetMarketClosePrice();
}

////////////////////////////////////////////////////////////////////////////////

StopLoss::Params::Params(const Volume &maxLossPerLot)
    : m_maxLossPerLot(maxLossPerLot) {}

const Volume &StopLoss::Params::GetMaxLossPerLot() const {
  return m_maxLossPerLot;
}

StopLoss::StopLoss(const boost::shared_ptr<const Params> &params,
                   Position &position,
                   PositionController &controller)
    : StopLossOrder(position, controller), m_params(params) {}

StopLoss::StopLoss(const boost::shared_ptr<const Params> &params,
                   const boost::posix_time::time_duration &delay,
                   Position &position,
                   PositionController &controller)
    : StopLossOrder(delay, position, controller), m_params(params) {}

const char *StopLoss::GetName() const { return "stop-loss"; }

void StopLoss::Report(const char *action) const {
  GetTradingLog().Write(
      "{'algo': {'action': '%6%', 'type': '%1%', 'params': {'maxLoss': "
      "%2$.8f}, 'delayTime': '%3%', 'operation': '%4%/%5%'}}",
      [this, action](TradingRecord &record) {
        record % GetName()                           // 1
            % m_params->GetMaxLossPerLot()           // 2
            % GetDelay()                             // 3
            % GetPosition().GetOperation()->GetId()  // 4
            % GetPosition().GetSubOperationId()      // 5
            % action;                                // 6
      });
}

bool StopLoss::Activate() {
  const Double maxLoss =
      GetPosition().GetOpenedQty() * -m_params->GetMaxLossPerLot();
  const auto &plannedPnl = GetPosition().GetPlannedPnl();

  if (maxLoss < plannedPnl) {
    return false;
  }

  GetTradingLog().Write(
      "%1%\thit"
      "\tplanned-pnl=%2$.8f\tmax-loss=%3$.8f*%4$.8f=%5$.8f"
      "\tbid/ask=%6$.8f/%7$.8f\tpos=%8%/%9%",
      [&](TradingRecord &record) {
        record % GetName()                                    // 1
            % plannedPnl                                      // 2
            % GetPosition().GetOpenedQty()                    // 3
            % m_params->GetMaxLossPerLot()                    // 4
            % maxLoss                                         // 5
            % GetPosition().GetSecurity().GetBidPriceValue()  // 6
            % GetPosition().GetSecurity().GetAskPriceValue()  // 7
            % GetPosition().GetOperation()->GetId()           // 8
            % GetPosition().GetSubOperationId();              // 9
      });

  return true;
}

////////////////////////////////////////////////////////////////////////////////

StopLossShare::Params::Params(const Double &maxLossShare)
    : m_maxLossShare(maxLossShare) {}

const Volume &StopLossShare::Params::GetMaxLossShare() const {
  return m_maxLossShare;
}

StopLossShare::StopLossShare(const boost::shared_ptr<const Params> &params,
                             Position &position,
                             PositionController &controller)
    : StopLossOrder(position, controller), m_params(params) {}

StopLossShare::StopLossShare(const Double &maxLossShare,
                             Position &position,
                             PositionController &controller)
    : StopLossOrder(position, controller),
      m_params(boost::make_shared<Params>(maxLossShare)) {}

const char *StopLossShare::GetName() const { return "stop-loss share"; }

void StopLossShare::Report(const char *action) const {
  GetTradingLog().Write(
      "{'algo': {'action': '%6%', 'type': '%1%', 'params': {'maxLoss': "
      "%2$.8f}, 'delayTime': '%3%', 'operation': '%4%/%5%'}}",
      [this, action](TradingRecord &record) {
        record % GetName()                           // 1
            % m_params->GetMaxLossShare()            // 2
            % GetDelay()                             // 3
            % GetPosition().GetOperation()->GetId()  // 4
            % GetPosition().GetSubOperationId()      // 5
            % action;                                // 6
      });
}

bool StopLossShare::Activate() {
  const auto &plannedPnl = GetPosition().GetPlannedPnl();
  const auto &openedVolume = GetPosition().GetOpenedVolume();
  const auto &maxLoss = openedVolume * -m_params->GetMaxLossShare();
  if (plannedPnl >= maxLoss) {
    return false;
  }

  GetTradingLog().Write(
      "%1%\thit"
      "\tplanned-pnl=%2$.8f\tmax-loss=%3$.8f*%4$.8f=%5$.8f"
      "\tbid/ask=%6$.8f/%7$.8f\tpos=%8%/%9%",
      [&](TradingRecord &record) {
        record % GetName()                                    // 1
            % plannedPnl                                      // 2
            % openedVolume                                    // 3
            % m_params->GetMaxLossShare()                     // 4
            % maxLoss                                         // 5
            % GetPosition().GetSecurity().GetBidPriceValue()  // 6
            % GetPosition().GetSecurity().GetAskPriceValue()  // 7
            % GetPosition().GetOperation()->GetId()           // 8
            % GetPosition().GetSubOperationId();              // 9
      });

  return true;
}

////////////////////////////////////////////////////////////////////////////////
