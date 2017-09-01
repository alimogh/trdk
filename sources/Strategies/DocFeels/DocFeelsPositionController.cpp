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
#include "TradingLib/StopLoss.hpp"
#include "Core/Position.hpp"
#include "DocFeelsTrend.hpp"

using namespace trdk;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::DocFeels;

PositionController::PositionController(Strategy &strategy, const Trend &trend)
    : Base(strategy), m_trend(trend) {}

Position &PositionController::OpenPosition(Security &security,
                                           const Milestones &delayMeasurement) {
  Assert(m_trend.IsExistent());
  return OpenPosition(security, m_trend.IsRising(), delayMeasurement);
}

Position &PositionController::OpenPosition(Security &security,
                                           bool isLong,
                                           const Milestones &delayMeasurement) {
  auto &result = Base::OpenPosition(security, isLong, delayMeasurement);
  result.AttachAlgo(std::make_unique<TradingLib::StopLossShare>(0.2, result));
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
  position.Close(position.GetMarketClosePrice(), reason);
}

Qty PositionController::GetNewPositionQty() const { return 1; }

bool PositionController::IsPositionCorrect(const Position &position) const {
  return m_trend.IsRising() == position.IsLong() || !m_trend.IsExistent();
}
