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
#include "Core/TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Services;
using namespace trdk::Strategies::MrigeshKejriwal;

namespace pt = boost::posix_time;
namespace mk = trdk::Strategies::MrigeshKejriwal;

////////////////////////////////////////////////////////////////////////////////

Trend::Trend() : m_isRising(boost::indeterminate) {}

bool Trend::Update(const Price &spotPrice,
                   const MovingAverageService::Point &ma) {
  if (spotPrice == ma.value) {
    return false;
  }
  const boost::tribool isRising(ma.value < spotPrice);
  if (isRising == m_isRising) {
    return false;
  }
  const bool isNewTrand = boost::indeterminate(m_isRising);
  m_isRising = std::move(isRising);
  return !isNewTrand;
}
////////////////////////////////////////////////////////////////////////////////

mk::Strategy::Settings::Settings(const IniSectionRef &conf)
    : qty(conf.ReadTypedKey<Qty>("qty")) {}

void mk::Strategy::Settings::Validate() const {
  if (qty < 1) {
    throw Exception("Position size is not set");
  }
}

void mk::Strategy::Settings::Log(Module::Log &log) const {
  boost::format info("Position size: %1%.");
  info % qty;  // 1
  log.Info(info.str().c_str());
}

////////////////////////////////////////////////////////////////////////////////

mk::Strategy::Strategy(Context &context,
                       const std::string &instanceName,
                       const IniSectionRef &conf,
                       const boost::shared_ptr<Trend> &trend)
    : Base(context,
           boost::uuids::string_generator()(
               "{ade86a15-32fe-41eb-a2b8-b7094e02669e}"),
           "MrigeshKejriwal",
           instanceName,
           conf),
      m_settings(conf),
      m_tradingSecurity(nullptr),
      m_spotSecurity(nullptr),
      m_ma(nullptr),
      m_trend(trend) {
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
      request.RequestTime(GetContext().GetCurrentTime() - pt::hours(3));
      m_tradingSecurity = &security;
      GetLog().Info("Using \"%1%\" to trade...", *m_tradingSecurity);
      break;
    case SECURITY_TYPE_INDEX:
      if (m_spotSecurity) {
        throw Exception(
            "Strategy can not work with more than one spot-security");
      }
      m_spotSecurity = &security;
      GetLog().Info("Using \"%1%\" to get spot price...", *m_spotSecurity);
      break;
    default:
      throw Exception("Strategy can not work with security with unknown type");
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
  AssertLt(0, position.GetNumberOfOpenOrders());

  if (position.IsCompleted()) {
    // No active order, no active qty...

    Assert(!position.HasActiveOrders());

    if (position.GetNumberOfCloseOrders()) {
      // Position fully closed. Maybe it better to open opposite position here
      // as position should be closed by signal?
      ReportOperation(position);
      return;
    }

    // Open order was canceled by some condition. Checking open
    // signal again and sending new open order...
    if (m_trend->IsRising() == position.IsLong()) {
      ContinuePosition(position);
    }

    // Position will be deleted if was not continued.

  } else if (position.GetNumberOfCloseOrders()) {
    // Position closing started.

    Assert(!position.HasActiveOpenOrders());
    AssertNe(CLOSE_REASON_NONE, position.GetCloseReason());

    if (position.HasActiveCloseOrders()) {
      // Closing in progress.
    } else if (position.GetCloseReason() != CLOSE_REASON_NONE) {
      // Close order was canceled by some condition. Sending
      // new close order.
      ClosePosition(position);
    }

  } else if (position.HasActiveOrders()) {
    // Opening in progress.

    Assert(!position.HasActiveCloseOrders());

    if (m_trend->IsRising() != position.IsLong()) {
      // Close signal received, closing position...
      ClosePosition(position);
    }

  } else if (m_trend->IsRising() == position.IsLong()) {
    // Holding position and waiting for close signal...
    AssertLt(0, position.GetActiveQty());
  } else {
    // Holding position and received close signal...
    AssertLt(0, position.GetActiveQty());
    ClosePosition(position);
  }
}

void mk::Strategy::OnLevel1Tick(Security &security,
                                const pt::ptime &,
                                const Level1TickValue &tick,
                                const Milestones &delayMeasurement) {
  if (tick.GetType() != LEVEL1_TICK_LAST_PRICE ||
      &security != &GetSpotSecurity() || GetMa().IsEmpty()) {
    return;
  }
  CheckSignal(security.DescalePrice(tick.GetValue()), delayMeasurement);
}

void mk::Strategy::OnServiceDataUpdate(const Service &, const Milestones &) {}

void mk::Strategy::OnPostionsCloseRequest() {
  throw MethodDoesNotImplementedError(
      "trdk::Strategies::MrigeshKejriwal::Strategy"
      "::OnPostionsCloseRequest is not implemented");
}

void mk::Strategy::CheckSignal(const Price &spotPrice,
                               const Milestones &delayMeasurement) {
  const auto &lastPoint = GetMa().GetLastPoint();
  if (!m_trend->Update(spotPrice, lastPoint)) {
    return;
  }
  Assert(!boost::indeterminate(m_trend->IsRising()));
  GetTradingLog().Write(
      "trend changed\tprice=%1%\tema=%2%\tspot-price=%3%\tnew-direction=%4%",
      [this, &lastPoint, &spotPrice](TradingRecord &record) {
        record % lastPoint.source                            // 1
            % lastPoint.value                                // 2
            % spotPrice                                      // 3
            % (m_trend->IsRising() ? "rising" : "falling");  // 4
      });

  Position *position = nullptr;
  if (!GetPositions().IsEmpty()) {
    AssertEq(1, GetPositions().GetSize());
    position = &*GetPositions().GetBegin();
    if (position->IsCompleted()) {
      position = nullptr;
    } else if (m_trend->IsRising() == position->IsLong()) {
      delayMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
      return;
    }
  }

  if (!position) {
    if (boost::indeterminate(m_trend->IsRising())) {
      return;
    }
  } else if (position->IsCancelling() || position->HasActiveCloseOrders()) {
    return;
  }

  delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_1);

  if (!position) {
    Assert(m_trend->IsRising() || !m_trend->IsRising());
    position = &OpenPosition(m_trend->IsRising(), delayMeasurement);
  } else if (position->HasActiveOpenOrders()) {
    try {
      Verify(position->CancelAllOrders());
    } catch (const TradingSystem::UnknownOrderCancelError &ex) {
      GetTradingLog().Write("failed to cancel order");
      GetLog().Warn("Failed to cancel order: \"%1%\".", ex.what());
      return;
    }
  } else {
    ClosePosition(*position);
  }

  delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);
}

Position &mk::Strategy::OpenPosition(bool isLong,
                                     const Milestones &delayMeasurement) {
  auto position =
      isLong ? CreatePosition<LongPosition>(
                   GetTradingSecurity().GetAskPriceScaled(), delayMeasurement)
             : CreatePosition<ShortPosition>(
                   GetTradingSecurity().GetBidPriceScaled(), delayMeasurement);
  ContinuePosition(*position);
  return *position;
}

void mk::Strategy::ContinuePosition(Position &position) {
  Assert(!position.HasActiveOrders());
  position.Open(position.GetMarketOpenPrice());
}

void mk::Strategy::ClosePosition(Position &position) {
  Assert(!position.HasActiveOrders());
  position.SetCloseReason(CLOSE_REASON_SIGNAL);
  position.Close(position.GetMarketClosePrice());
}

void mk::Strategy::ReportOperation(const Position &pos) {
  if (!m_strategyLog.is_open()) {
    m_strategyLog = OpenDataLog("csv");
    m_strategyLog << "Date,Open Start Time,Open Time,Opening Duration"
                  << ",Close Start Time,Close Time,Closing Duration"
                  << ",Position Duration,Type,P&L Volume,P&L %,Is Profit"
                  << ",Is Loss,Qty,Open Price,Open Orders,Open Trades"
                  << ",Close Reason,Close Price,Close Orders, Close Trades,ID"
                  << std::endl;
  }

  m_strategyLog << pos.GetOpenStartTime().date() << ','
                << ExcelTextField(pos.GetOpenStartTime().time_of_day()) << ','
                << ExcelTextField(pos.GetOpenTime().time_of_day()) << ','
                << ExcelTextField(pos.GetOpenTime() - pos.GetOpenStartTime())
                << ',' << ExcelTextField(pos.GetCloseStartTime().time_of_day())
                << ',' << ExcelTextField(pos.GetCloseTime().time_of_day())
                << ','
                << ExcelTextField(pos.GetCloseTime() - pos.GetCloseStartTime())
                << ',' << ExcelTextField(pos.GetCloseTime() - pos.GetOpenTime())
                << ',' << pos.GetType() << ',' << pos.GetRealizedPnlVolume()
                << ',' << pos.GetRealizedPnlRatio()
                << (pos.IsProfit() ? ",1,0" : ",0,1") << ','
                << pos.GetOpenedQty() << ','
                << GetTradingSecurity().DescalePrice(pos.GetOpenAvgPrice())
                << ',' << pos.GetNumberOfOpenOrders() << ','
                << pos.GetNumberOfOpenTrades() << ',' << pos.GetCloseReason()
                << ','
                << GetTradingSecurity().DescalePrice(pos.GetCloseAvgPrice())
                << ',' << pos.GetNumberOfCloseOrders() << ','
                << pos.GetNumberOfCloseTrades() << ',' << pos.GetId()
                << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
