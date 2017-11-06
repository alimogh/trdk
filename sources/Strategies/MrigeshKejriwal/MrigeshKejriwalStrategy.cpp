/*******************************************************************************
 *   Created: 2017/07/19 20:43:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MrigeshKejriwalStrategy.hpp"
#include "TradingLib/OrderPolicy.hpp"
#include "Core/TradingLog.hpp"
#include "MrigeshKejriwalPositionOperationContext.hpp"
#include "MrigeshKejriwalPositionReport.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Services;
using namespace trdk::Strategies::MrigeshKejriwal;

namespace pt = boost::posix_time;
namespace mk = trdk::Strategies::MrigeshKejriwal;
namespace tl = trdk::TradingLib;

////////////////////////////////////////////////////////////////////////////////

bool Trend::Update(const Price &lastPrice, double controlValue) {
  if (lastPrice == controlValue) {
    return false;
  }
  return SetIsRising(controlValue < lastPrice);
}

////////////////////////////////////////////////////////////////////////////////

PositionController::PositionController(trdk::Strategy &strategy,
                                       const mk::Settings &settings)
    : Base(strategy), m_settings(settings) {}

std::unique_ptr<tl::PositionReport> PositionController::OpenReport() const {
  return boost::make_unique<PositionReport>(GetStrategy(), m_settings);
}

////////////////////////////////////////////////////////////////////////////////

mk::Strategy::Strategy(Context &context,
                       const std::string &instanceName,
                       const IniSectionRef &conf,
                       const boost::shared_ptr<Trend> &trend)
    : Base(context,
           "{ade86a15-32fe-41eb-a2b8-b7094e02669e}",
           "MrigeshKejriwal",
           instanceName,
           conf),
      m_settings(conf),
      m_skipNextSignal(true),
      m_tradingSecurity(nullptr),
      m_underlyingSecurity(nullptr),
      m_ma(nullptr),
      m_trend(trend),
      m_positionController(*this, m_settings) {
  m_settings.Log(GetLog());
  m_settings.Validate();
}

void mk::Strategy::OnSecurityStart(Security &security,
                                   Security::Request &request) {
  Base::OnSecurityStart(security, request);
  switch (security.GetSymbol().GetSecurityType()) {
    case SECURITY_TYPE_FUTURES:
      if (m_tradingSecurity) {
        throw Exception(
            "Strategy can not work with more than one trading security");
      }
      m_tradingSecurity = &security;
      GetLog().Info("Using \"%1%\" (%2%) to trade...", *m_tradingSecurity,
                    m_tradingSecurity->GetExpiration().GetDate());
      break;
    case SECURITY_TYPE_INDEX:
      if (m_underlyingSecurity) {
        throw Exception(
            "Strategy can not work with more than one underlying-security");
      }
      request.RequestTime(GetContext().GetCurrentTime() -
                          pt::hours(m_settings.numberOfHistoryHours));
      m_underlyingSecurity = &security;
      GetLog().Info("Using \"%1%\" to get spot price...",
                    *m_underlyingSecurity);
      break;
    default:
      throw Exception("Strategy can not work with security with unknown type");
  }
}

void mk::Strategy::OnSecurityContractSwitched(const pt::ptime &,
                                              Security &security,
                                              Security::Request &request,
                                              bool &isSwitched) {
  Assert(&security == &GetTradingSecurity());
  if (&security != &GetTradingSecurity()) {
    return;
  } else if (!isSwitched) {
    return;
  } else if (!StartRollOver()) {
    request.RequestTime(GetContext().GetCurrentTime() -
                        pt::hours(m_settings.numberOfHistoryHours));
    GetLog().Info("Using new contract \"%1%\" (%2%) to trade...",
                  GetTradingSecurity(),
                  GetTradingSecurity().GetExpiration().GetDate());
    if (m_rollover) {
      m_rollover->isCompleted = true;
    }
  } else {
    isSwitched = false;
  }
}

void mk::Strategy::OnBrokerPositionUpdate(Security &security,
                                          bool isLong,
                                          const Qty &qty,
                                          const Volume &volume,
                                          bool isInitial) {
  Assert(&security == &GetTradingSecurity());
  if (&security != &GetTradingSecurity()) {
    GetLog().Error(
        "Wrong wrong broker position \"%5%\" %1% (volume %2$.8f) for \"%3%\" "
        "(%4%).",
        qty,                               // 1
        volume,                            // 2
        security,                          // 3
        isInitial ? "initial" : "online",  // 4
        isLong ? "long" : "short");        // 5
    throw Exception("Broker position for wrong security");
  }
  auto posQty = qty;
  if (posQty != 0 && posQty < m_settings.minQty) {
    posQty = m_settings.minQty;
  }
  const auto numberOfParts =
      static_cast<intmax_t>(posQty) / static_cast<intmax_t>(m_settings.minQty);
  const auto restFromPart =
      static_cast<intmax_t>(posQty) % static_cast<intmax_t>(m_settings.minQty);
  posQty = static_cast<double>((numberOfParts + (restFromPart ? 1 : 0)) *
                               static_cast<intmax_t>(m_settings.minQty));
  const auto posVolume = (volume / qty) * posQty;
  m_positionController.OnBrokerPositionUpdate(
      boost::make_shared<PositionOperationContext>(m_settings, *m_trend,
                                                   volume / qty),
      security, isLong, posQty, posVolume, isInitial);

  if (isInitial && !GetPositions().IsEmpty()) {
    Assert(m_skipNextSignal);
    m_skipNextSignal = false;
  }
}

void mk::Strategy::OnServiceStart(const Service &service) {
  const auto *const ma = dynamic_cast<const MovingAverageService *>(&service);
  if (ma) {
    OnMaServiceStart(*ma);
  }
}

void mk::Strategy::OnMaServiceStart(const MovingAverageService &service) {
  if (m_ma) {
    throw Exception(
        "Strategy uses one Moving Average service, but configured more");
  }
  m_ma = &service;
  GetLog().Info("Using Moving Average service \"%1%\"...", *m_ma);
}

void mk::Strategy::OnPositionUpdate(Position &position) {
  if (ContinueRollOver()) {
    return;
  }

  m_positionController.OnPositionUpdate(position);

  if (position.IsCompleted() &&
      position.GetCloseReason() == CLOSE_REASON_STOP_LOSS &&
      GetPositions().GetSize() == 1) {
    Assert(!m_priceSignal);
    m_priceSignal = PriceSignal{position.IsLong(), position.GetCloseAvgPrice()};
  }
}

void mk::Strategy::OnLevel1Tick(Security &security,
                                const pt::ptime &time,
                                const Level1TickValue &tick,
                                const Milestones &delayMeasurement) {
  if (&security != &GetTradingSecurity()) {
    return;
  }
  if (tick.GetType() != LEVEL1_TICK_LAST_PRICE || GetMa().IsEmpty()) {
    return;
  }
  if (FinishRollOver() || ContinueRollOver()) {
    return;
  }

  m_lastPrices.emplace_back(time, tick.GetValue());
  while (!m_lastPrices.empty() &&
         m_lastPrices.front().first + m_settings.pricesPeriod < time) {
    m_lastPrices.pop_front();
  }
  AssertLe(1, m_lastPrices.size());

  Price sum = 0;
  for (const auto &price : m_lastPrices) {
    sum += price.second;
  }
  CheckSignal(sum / m_lastPrices.size(), delayMeasurement);
}

void mk::Strategy::OnServiceDataUpdate(const Service &, const Milestones &) {}

void mk::Strategy::OnPostionsCloseRequest() {
  m_positionController.OnPostionsCloseRequest();
}

void mk::Strategy::CheckSignal(const trdk::Price &signalPrice,
                               const Milestones &delayMeasurement) {
  const auto numberOfDaysToExpiry =
      (GetTradingSecurity().GetExpiration().GetDate() -
       GetContext().GetCurrentTime().date())
          .days();
  const auto &lastPoint = GetMa().GetLastPoint();
  const auto controlValue =
      lastPoint.value *
      (1 + m_settings.costOfFunds * numberOfDaysToExpiry / 365);
  bool isTrendChanged = m_trend->Update(signalPrice, controlValue);
  GetTradingLog().Write(
      "trend\t%1%\tdirection=%2%\tsignal-price=%3%\tcontrol-price=%4%"
      "\tcontrol-price-ema=%5%\tdays-to-expiry=%6%\tcontol-value=%7%"
      "\ttr-bid/ask=%8%/%9%\tund-bid/ask=%10%/%11%",
      [&](TradingRecord &record) {
        record % (isTrendChanged ? "CHANGED" : "-------")  // 1
            % (m_trend->IsRising()
                   ? "rising"
                   : !m_trend->IsRising() ? "falling" : "-------")  // 2
            % signalPrice                                           // 3
            % lastPoint.source                                      // 4
            % lastPoint.value                                       // 5
            % numberOfDaysToExpiry                                  // 6
            % controlValue                                          // 7
            % m_tradingSecurity->GetBidPriceValue()                 // 8
            % m_tradingSecurity->GetAskPriceValue()                 // 9
            % m_underlyingSecurity->GetBidPriceValue()              // 10
            % m_underlyingSecurity->GetAskPriceValue();             // 11
      });
  if (!isTrendChanged && m_priceSignal) {
    isTrendChanged = m_priceSignal->isLong ? m_priceSignal->price < signalPrice
                                           : m_priceSignal->price > signalPrice;
  }
  if (!isTrendChanged) {
    return;
  }

  m_priceSignal = boost::none;

  if (m_skipNextSignal) {
    m_skipNextSignal = false;
    return;
  }

  auto *const changedPosition = m_positionController.OnSignal(
      boost::make_shared<PositionOperationContext>(m_settings, *m_trend,
                                                   signalPrice),
      GetTradingSecurity(), delayMeasurement);
  if (changedPosition &&
      changedPosition->GetCloseReason() == CLOSE_REASON_SIGNAL) {
    boost::polymorphic_cast<PositionOperationContext *>(
        &changedPosition->GetOperationContext())
        ->SetCloseSignalPrice(signalPrice);
  }
}

bool mk::Strategy::StartRollOver() {
  if (m_rollover) {
    return !GetPositions().IsEmpty() &&
           !GetPositions().GetBegin()->IsCompleted();
  }
  if (GetPositions().IsEmpty()) {
    return false;
  }
  AssertEq(1, GetPositions().GetSize());

  Position &position = *GetPositions().GetBegin();
  if (position.HasActiveCloseOrders()) {
    return false;
  }

  GetLog().Info("Starting rollover...");
  Assert(&position.GetSecurity() == &GetTradingSecurity());
  m_rollover = Rollover{false, position.IsLong(), position.GetOpenedQty()};

  return true;
}

bool mk::Strategy::ContinueRollOver() {
  if (!m_rollover || GetPositions().IsEmpty()) {
    return false;
  }

  Position &position = *GetPositions().GetBegin();
  if (position.HasActiveCloseOrders()) {
    return false;
  }
  if (GetPositions().GetBegin()->IsCompleted()) {
    position.GetSecurity().ContinueContractSwitchingToNextExpiration();
    return true;
  }

  GetTradingLog().Write("rollover\tposition-expiration=%1%\tposition=%2%",
                        [&position](TradingRecord &record) {
                          record % position.GetExpiration().GetDate()  // 1
                              % position.GetId();                      // 2
                        });
  m_rollover->isStarted = true;

  if (position.HasActiveOpenOrders()) {
    try {
      position.CancelAllOrders();
    } catch (const TradingSystem::OrderIsUnknown &ex) {
      GetLog().Warn("Failed to cancel order: \"%1%\".", ex.what());
    }
    return false;
  }
  position.ResetCloseReason(CLOSE_REASON_ROLLOVER);
  m_positionController.ClosePosition(position, CLOSE_REASON_ROLLOVER);

  return true;
}

bool mk::Strategy::FinishRollOver() {
  if (!m_rollover || !m_rollover->isCompleted) {
    return false;
  }
  GetLog().Info("Finishing rollover...");
  AssertEq(0, GetPositions().GetSize());
  m_positionController.OpenPosition(
      boost::make_shared<PositionOperationContext>(m_settings, *m_trend, 0),
      GetTradingSecurity(), m_rollover->isLong, m_rollover->qty, Milestones());
  m_rollover = boost::none;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
