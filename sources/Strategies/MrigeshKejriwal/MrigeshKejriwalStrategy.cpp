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
#include "TradingLib/StopLoss.hpp"
#include "Core/TradingLog.hpp"
#include "MrigeshKejriwalOrderPolicy.hpp"
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
                                       const Trend &trend,
                                       const mk::Settings &settings)
    : Base(strategy),
      m_settings(settings),
      m_orderPolicy(m_settings.orderPolicyFactory->CreateOrderPolicy()),
      m_trend(trend) {}

Position &PositionController::OpenPosition(Security &security,
                                           const Milestones &delayMeasurement) {
  Assert(m_trend.IsExistent());
  return OpenPosition(security, m_trend.IsRising(), delayMeasurement);
}

Position &PositionController::OpenPosition(Security &security,
                                           bool isLong,
                                           const Milestones &delayMeasurement) {
  auto &result = Base::OpenPosition(security, isLong, delayMeasurement);
  if (m_settings.maxLossShare != 0) {
    result.AttachAlgo(std::make_unique<tl::StopLossShare>(
        m_settings.maxLossShare, result, m_orderPolicy));
  }
  return result;
}

Qty PositionController::GetNewPositionQty() const { return m_settings.qty; }

bool PositionController::IsPositionCorrect(const Position &position) const {
  return m_trend.IsRising() == position.IsLong() || !m_trend.IsExistent();
}

const tl::OrderPolicy &PositionController::GetOpenOrderPolicy() const {
  return *m_orderPolicy;
}
const tl::OrderPolicy &PositionController::GetCloseOrderPolicy() const {
  return *m_orderPolicy;
}

std::unique_ptr<tl::PositionReport> PositionController::OpenReport() const {
  return boost::make_unique<PositionReport>(GetStrategy());
}

////////////////////////////////////////////////////////////////////////////////

class mk::Settings::LimitGtcOrderPolicyFactory
    : public mk::Settings::OrderPolicyFactory {
 public:
  explicit LimitGtcOrderPolicyFactory(const Price &correction)
      : m_correction(correction) {}
  virtual ~LimitGtcOrderPolicyFactory() override = default;
  virtual std::unique_ptr<tl::OrderPolicy> CreateOrderPolicy() const override {
    return boost::make_unique<LimitOrderPolicy<tl::LimitGtcOrderPolicy>>(
        m_correction);
  }

 private:
  const Price m_correction;
};

class mk::Settings::LimitIocOrderPolicyFactory
    : public mk::Settings::OrderPolicyFactory {
 public:
  explicit LimitIocOrderPolicyFactory(const Price &correction)
      : m_correction(correction) {}
  virtual ~LimitIocOrderPolicyFactory() override = default;
  virtual std::unique_ptr<tl::OrderPolicy> CreateOrderPolicy() const override {
    return boost::make_unique<LimitOrderPolicy<tl::LimitIocOrderPolicy>>(
        m_correction);
  }

 private:
  const Price m_correction;
};

class mk::Settings::MarketGtcOrderPolicyFactory
    : public mk::Settings::OrderPolicyFactory {
 public:
  virtual ~MarketGtcOrderPolicyFactory() override = default;
  virtual std::unique_ptr<tl::OrderPolicy> CreateOrderPolicy() const override {
    return boost::make_unique<tl::MarketGtcOrderPolicy>();
  }
};

mk::Settings::Settings(const IniSectionRef &conf)
    : qty(conf.ReadTypedKey<Qty>("qty")),
      minQty(conf.ReadTypedKey<Qty>("qty_min")),
      numberOfHistoryHours(conf.ReadTypedKey<uint16_t>("history_hours")),
      costOfFunds(conf.ReadTypedKey<double>("cost_of_funds")),
      maxLossShare(conf.ReadTypedKey<double>("max_loss_share")),
      signalPriceCorrection(
          conf.ReadTypedKey<Price>("signal_price_correction")) {
  {
    const auto &orderType = conf.ReadKey("order_type");
    if (boost::iequals(orderType, "lmt_gtc")) {
      orderPolicyFactory =
          boost::make_unique<LimitGtcOrderPolicyFactory>(signalPriceCorrection);
    } else if (boost::iequals(orderType, "lmt_ioc")) {
      orderPolicyFactory =
          boost::make_unique<LimitIocOrderPolicyFactory>(signalPriceCorrection);
    } else if (boost::iequals(orderType, "mkt_gtc")) {
      orderPolicyFactory = boost::make_unique<MarketGtcOrderPolicyFactory>();
    } else {
      throw Exception(
          "Unknown order type is set (allowed \"lmt_gtc\", \"lmt_ioc\" or "
          "\"mkt_gtc\")");
    }
  }
}

void mk::Settings::Validate() const {
  if (qty < 1) {
    throw Exception("Position size is not set");
  }
  if (minQty < 1) {
    throw Exception("Min. order size is not set");
  }
}

void mk::Settings::Log(Module::Log &log) const {
  log.Info(
      "Position size: %1%. Min. order size: %2%. Number of history hours: %3%. "
      "Cost of funds: %4%. Max. share of loss: %5%. Signal price correction: "
      "%6%. Order type: %7%.",
      qty,                    // 1
      minQty,                 // 2
      numberOfHistoryHours,   // 3
      costOfFunds,            // 4
      maxLossShare,           // 5
      signalPriceCorrection,  // 6
      dynamic_cast<const LimitGtcOrderPolicyFactory *>(&*orderPolicyFactory)
          ? "limit GTC"
          : dynamic_cast<const LimitIocOrderPolicyFactory *>(
                &*orderPolicyFactory)
                ? "limit IOC"
                : dynamic_cast<const MarketGtcOrderPolicyFactory *>(
                      &*orderPolicyFactory)
                      ? "market GTC"
                      : "UNKNOWN!");  // 7
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
      m_positionController(*this, *m_trend, m_settings) {
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
  if (posQty != 0 && abs(posQty) < m_settings.minQty) {
    posQty = m_settings.minQty;
    if (qty < 0) {
      posQty *= -1;
    }
  }
  const auto numberOfParts =
      static_cast<intmax_t>(posQty) / static_cast<intmax_t>(m_settings.minQty);
  const auto restFromPart =
      static_cast<intmax_t>(posQty) % static_cast<intmax_t>(m_settings.minQty);
  posQty = static_cast<double>((numberOfParts + (restFromPart ? 1 : 0)) *
                               static_cast<intmax_t>(m_settings.minQty));
  const auto posVolume = (volume / abs(qty)) * posQty;
  m_positionController.OnBrokerPositionUpdate(security, posQty, posVolume,
                                              isInitial);

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
}

void mk::Strategy::OnLevel1Tick(Security &security,
                                const pt::ptime &,
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
  } else if (m_skipNextSignal) {
    m_skipNextSignal = false;
    return;
  }

  m_positionController.OnSignal(GetTradingSecurity(), delayMeasurement);
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
    } catch (const TradingSystem::UnknownOrderCancelError &ex) {
      GetLog().Warn("Failed to cancel order: \"%1%\".", ex.what());
    }
    return false;
  }
  m_positionController.ClosePosition(position, CLOSE_REASON_ROLLOVER);

  return true;
}

bool mk::Strategy::FinishRollOver() {
  if (!m_rollover || !m_rollover->isCompleted) {
    return false;
  }
  GetLog().Info("Finishing rollover...");
  AssertEq(0, GetPositions().GetSize());
  m_positionController.OpenPosition(GetTradingSecurity(), m_rollover->isLong,
                                    m_rollover->qty, Milestones());
  m_rollover = boost::none;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
