/*******************************************************************************
 *   Created: 2017/08/26 22:39:14
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "DocFeelsPositionController.hpp"
#include "TradingLib/OrderPolicy.hpp"
#include "TradingLib/StopLoss.hpp"
#include "Core/Position.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "DocFeelsPositionReport.hpp"
#include "DocFeelsTrend.hpp"

using namespace trdk;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::DocFeels;

namespace tl = trdk::TradingLib;

PositionController::PositionController(Strategy &strategy,
                                       const tl::Trend &trend,
                                       const Qty &startQty)
    : Base(strategy),
      m_orderPolicy(boost::make_shared<tl::LimitGtcOrderPolicy>()),
      m_trend(trend),
      m_qty(startQty) {}

void PositionController::OnPositionUpdate(Position &position) {
  if (position.IsCompleted() && position.GetOpenedQty() > 0) {
    if (position.IsProfit()) {
      if (position.GetRealizedPnlPercentage() > 25) {
        // 1.	If CTS - II’s Profit > 25 % , increase order volume(OV) with
        // 10 % of profits
        const auto usedPartOfProfit = position.GetRealizedPnlVolume() * 0.1;
        const auto usedPartOfProfitQty =
            usedPartOfProfit /
            position.GetSecurity().DescalePrice(position.GetCloseAvgPrice());
        position.GetStrategy().GetTradingLog().Write(
            "qty+\t%1%: %2%%% -> %3% -> %4% -> %5% = %6%",
            [&](TradingRecord &record) {
              record % m_qty                             // 1
                  % position.GetRealizedPnlPercentage()  // 2
                  % position.GetRealizedPnlVolume()      // 3
                  % usedPartOfProfit                     // 4
                  % usedPartOfProfitQty                  // 5
                  % (m_qty + usedPartOfProfitQty);       // 6
            });
        m_qty += usedPartOfProfitQty;
      }
    } else if (position.GetRealizedPnlPercentage() < -25) {
      const auto usedPartOfProfit = position.GetRealizedPnlVolume() * 0.1;
      const auto usedPartOfProfitQty =
          usedPartOfProfit /
          position.GetSecurity().DescalePrice(position.GetCloseAvgPrice());
      position.GetStrategy().GetTradingLog().Write(
          "qty-\t%1%: %2%%% -> %3% -> %4% -> %5% = %6%",
          [&](TradingRecord &record) {
            record % m_qty                             // 1
                % position.GetRealizedPnlPercentage()  // 2
                % position.GetRealizedPnlVolume()      // 3
                % usedPartOfProfit                     // 4
                % usedPartOfProfitQty                  // 5
                % (m_qty + usedPartOfProfitQty);       // 6
          });
      // 2.	If CTS - II’s Loss < -25 % , decrease OV by 10 %
      m_qty += usedPartOfProfitQty;
    }
  }
  Base::OnPositionUpdate(position);
}

Position &PositionController::OpenPosition(Security &security,
                                           const Milestones &delayMeasurement) {
  Assert(m_trend.IsExistent());
  return OpenPosition(security, m_trend.IsRising(), delayMeasurement);
}

Position &PositionController::OpenPosition(Security &security,
                                           bool isLong,
                                           const Milestones &delayMeasurement) {
  auto &result = Base::OpenPosition(security, isLong, delayMeasurement);
  result.AttachAlgo(
      std::make_unique<tl::StopLossShare>(0.2, result, m_orderPolicy));
  return result;
}

const tl::OrderPolicy &PositionController::GetOpenOrderPolicy() const {
  return *m_orderPolicy;
}
const tl::OrderPolicy &PositionController::GetCloseOrderPolicy() const {
  return *m_orderPolicy;
}

Qty PositionController::GetNewPositionQty() const { return m_qty; }

bool PositionController::IsPositionCorrect(const Position &position) const {
  return m_trend.IsRising() == position.IsLong() || !m_trend.IsExistent();
}

std::unique_ptr<tl::PositionReport> PositionController::OpenReport() const {
  return boost::make_unique<PositionReport>(GetStrategy());
}

const PositionReport &PositionController::GetReport() const {
  return *boost::polymorphic_downcast<const PositionReport *>(
      &Base::GetReport());
}
