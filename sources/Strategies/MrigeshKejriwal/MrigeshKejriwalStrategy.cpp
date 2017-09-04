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
#include "TradingLib/StopLoss.hpp"
#include "Core/TradingLog.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Services;
using namespace trdk::Strategies::MrigeshKejriwal;

namespace pt = boost::posix_time;
namespace mk = trdk::Strategies::MrigeshKejriwal;

////////////////////////////////////////////////////////////////////////////////

bool Trend::Update(const Price &lastPrice, double controlValue) {
  if (lastPrice == controlValue) {
    return false;
  }
  return SetIsRising(controlValue < lastPrice);
}

////////////////////////////////////////////////////////////////////////////////

PositionController::PositionController(trdk::Strategy &strategy,
                                       const Trend &trend,
                                       const mk::Settings &settings)
    : Base(strategy), m_trend(trend), m_settings(settings) {}

Position &PositionController::OpenPosition(Security &security,
                                           const Milestones &delayMeasurement) {
  Assert(m_trend.IsExistent());
  return OpenPosition(security, m_trend.IsRising(), delayMeasurement);
}

Position &PositionController::OpenPosition(Security &security,
                                           bool isLong,
                                           const Milestones &delayMeasurement) {
  auto &result = Base::OpenPosition(security, isLong, delayMeasurement);
  result.AttachAlgo(std::make_unique<TradingLib::StopLossShare>(
      m_settings.maxLossShare, result));
  return result;
}

void PositionController::ContinuePosition(Position &position) {
  Assert(!position.HasActiveOrders());
  position.Open(position.GetMarketOpenPrice());
}

void PositionController::ClosePosition(Position &position,
                                       const CloseReason &reason) {
  Assert(!position.HasActiveOrders());
  position.SetCloseReason(reason);
  position.Close(position.GetMarketClosePrice());
}

Qty PositionController::GetNewPositionQty() const { return m_settings.qty; }

bool PositionController::IsPositionCorrect(const Position &position) const {
  return m_trend.IsRising() == position.IsLong() || !m_trend.IsExistent();
}

////////////////////////////////////////////////////////////////////////////////

mk::Settings::Settings(const IniSectionRef &conf)
    : qty(conf.ReadTypedKey<Qty>("qty")),
      minQty(conf.ReadTypedKey<Qty>("qty_min")),
      numberOfHistoryHours(conf.ReadTypedKey<uint16_t>("history_hours")),
      costOfFunds(conf.ReadTypedKey<double>("cost_of_funds")),
      maxLossShare(conf.ReadTypedKey<double>("max_loss_share")) {}

void mk::Settings::Validate() const {
  if (qty < 1) {
    throw Exception("Position size is not set");
  }
}

void mk::Settings::Log(Module::Log &log) const {
  boost::format info(
      "Position size: %1%. Min. order size: %2%. Number of history hours: %2%. "
      "Cost of funds: %3%. Max. share of loss: %4%.");
  info % qty                  // 1
      % minQty                // 2
      % numberOfHistoryHours  // 3
      % costOfFunds           // 4
      % maxLossShare;         // 5
  log.Info(info.str().c_str());
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
      m_tradingSecurity(nullptr),
      m_underlyingSecurity(nullptr),
      m_ma(nullptr),
      m_trend(trend),
      m_positionController(*this, *m_trend, m_settings),
      m_isRollover(false),
      m_isTradingSecurityActivationReported(false) {
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
                                              Security::Request &request) {
  Assert(&security == &GetTradingSecurity());
  if (&security != &GetTradingSecurity()) {
    return;
  }
  request.RequestTime(GetContext().GetCurrentTime() -
                      pt::hours(m_settings.numberOfHistoryHours));
  GetLog().Info("Using new contract \"%1%\" (%2%) to trade...",
                GetTradingSecurity(),
                GetTradingSecurity().GetExpiration().GetDate());
  StartRollOver();
}

void mk::Strategy::OnBrokerPositionUpdate(Security &security,
                                          const Qty &qty,
                                          const Volume &volume,
                                          bool isInitial) {
  Assert(&security == &GetTradingSecurity());
  if (&security != &GetTradingSecurity()) {
    GetLog().Error(
        "Wrong wrong broker position %1% (volume %2$.8f) for \"%3%\" (%4%).",
        qty,                                // 1
        volume,                             // 2
        security,                           // 3
        isInitial ? "initial" : "online");  // 4
    throw Exception("Broker position for wrong security");
  }
  auto posQty = qty;
  if (abs(posQty) < m_settings.minQty) {
    posQty = m_settings.minQty;
    if (qty < 0) {
      posQty *= -1;
    }
  }
  m_positionController.OnBrokerPositionUpdate(security, posQty, volume,
                                              isInitial);
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
  FinishRollOver(position);
  m_positionController.OnPositionUpdate(position);
}

void mk::Strategy::OnLevel1Tick(Security &security,
                                const pt::ptime &,
                                const Level1TickValue &tick,
                                const Milestones &delayMeasurement) {
  if (&security != &GetTradingSecurity()) {
    return;
  }
  if (FinishRollOver()) {
    return;
  }
  if (tick.GetType() != LEVEL1_TICK_LAST_PRICE || GetMa().IsEmpty()) {
    return;
  } else if (!GetTradingSecurity().IsActive()) {
    if (!m_isTradingSecurityActivationReported) {
      GetLog().Warn(
          "Trading security \"%1%\" is not active (%2%/%3%).",
          GetTradingSecurity(),
          GetTradingSecurity().IsOnline() ? "online" : "offline",
          GetTradingSecurity().IsTradingSessionOpened() ? "opened" : "closed");
      m_isTradingSecurityActivationReported = true;
    }
    return;
  } else {
    m_isTradingSecurityActivationReported = false;
  }
  CheckSignal(GetTradingSecurity().DescalePrice(tick.GetValue()),
              delayMeasurement);
}

void mk::Strategy::OnServiceDataUpdate(const Service &, const Milestones &) {}

void mk::Strategy::OnPostionsCloseRequest() {
  m_positionController.OnPostionsCloseRequest();
}

void mk::Strategy::CheckSignal(const trdk::Price &tradePrice,
                               const Milestones &delayMeasurement) {
  const auto numberOfDaysToExpiry =
      (GetTradingSecurity().GetExpiration().GetDate() -
       GetContext().GetCurrentTime().date())
          .days();
  const auto &lastPoint = GetMa().GetLastPoint();
  const auto controlValue =
      lastPoint.value *
      (1 + m_settings.costOfFunds * numberOfDaysToExpiry / 365);
  const bool isTrendChanged = m_trend->Update(tradePrice, controlValue);
  GetTradingLog().Write(
      "trend\t%1%\tdirection=%2%\tlast-price=%3%\tclose-price=%4%"
      "\tclose-price-ema=%5%\tdays-to-expiry=%6%\tcontol=%7%",
      [&](TradingRecord &record) {
        record % (isTrendChanged ? "CHANGED" : "-------")  // 1
            % (m_trend->IsRising()
                   ? "rising"
                   : !m_trend->IsRising() ? "falling" : "-------")  // 2
            % tradePrice                                            // 3
            % lastPoint.source                                      // 4
            % lastPoint.value                                       // 5
            % numberOfDaysToExpiry                                  // 6
            % controlValue;                                         // 7
      });
  if (!isTrendChanged) {
    return;
  }

  m_positionController.OnSignal(GetTradingSecurity(), delayMeasurement);
}

bool mk::Strategy::StartRollOver() {
  if (m_isRollover) {
    return false;
  }

  if (!GetPositions().GetSize()) {
    return false;
  }
  AssertEq(1, GetPositions().GetSize());

  Position &position = *GetPositions().GetBegin();
  if (position.HasActiveCloseOrders()) {
    return false;
  }

  const Security &security = GetTradingSecurity();
  Assert(security.HasExpiration());
  if (position.GetExpiration() == security.GetExpiration()) {
    return false;
  }

  GetTradingLog().Write("rollover\texpiration=%1%\tposition=%2%",
                        [&security, &position](TradingRecord &record) {
                          record % security.GetExpiration().GetDate() %
                              position.GetId();
                        });

  if (position.HasActiveOpenOrders()) {
    try {
      position.CancelAllOrders();
    } catch (const TradingSystem::UnknownOrderCancelError &ex) {
      GetLog().Warn("Failed to cancel order: \"%1%\".", ex.what());
    }
    return false;
  }

  position.SetCloseReason(CLOSE_REASON_ROLLOVER);
  position.CloseAtMarketPrice();

  m_isRollover = true;
  return true;
}

void mk::Strategy::CancelRollOver() { m_isRollover = false; }

bool mk::Strategy::FinishRollOver() {
  if (!GetPositions().GetSize()) {
    return false;
  }
  AssertEq(1, GetPositions().GetSize());
  return FinishRollOver(*GetPositions().GetBegin());
}

bool mk::Strategy::FinishRollOver(Position &oldPosition) {
  if (!m_isRollover || !oldPosition.IsCompleted()) {
    return false;
  } else if (!GetTradingSecurity().IsActive()) {
    if (!m_isTradingSecurityActivationReported) {
      GetLog().Warn(
          "Postponing rollover finishing as \"%1%\" is not active (%2%/%3%).",
          GetTradingSecurity(),
          GetTradingSecurity().IsOnline() ? "online" : "offline",
          GetTradingSecurity().IsTradingSessionOpened() ? "opened" : "closed");
      m_isTradingSecurityActivationReported = true;
    }
  } else {
    m_isTradingSecurityActivationReported = false;
  }
  m_positionController.OpenPosition(GetTradingSecurity(), oldPosition.IsLong(),
                                    Milestones());
  m_isRollover = false;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
