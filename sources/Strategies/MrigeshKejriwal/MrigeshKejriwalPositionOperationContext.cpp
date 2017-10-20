/*******************************************************************************
 *   Created: 2017/10/12 23:12:03
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MrigeshKejriwalPositionOperationContext.hpp"
#include "TradingLib/StopLoss.hpp"
#include "MrigeshKejriwalStrategy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::MrigeshKejriwal;

namespace mk = trdk::Strategies::MrigeshKejriwal;

mk::PositionOperationContext::PositionOperationContext(
    const Settings &settings, const Trend &trend, const Price &openSignalPrice)
    : m_settings(settings),
      m_trend(trend),
      m_openSignalPrice(openSignalPrice),
      m_closeSignalPrice(0) {
  AssertNe(0, m_openSignalPrice);
}

const OrderPolicy &mk::PositionOperationContext::GetOpenOrderPolicy() const {
  return *m_settings.orderPolicy;
}

const OrderPolicy &mk::PositionOperationContext::GetCloseOrderPolicy() const {
  return *m_settings.orderPolicy;
}

void mk::PositionOperationContext::Setup(Position &position) const {
  if (m_settings.maxLossShare != 0) {
    position.AttachAlgo(std::make_unique<TradingLib::StopLossShare>(
        m_settings.maxLossShare, position, m_settings.orderPolicy));
  }
}

bool mk::PositionOperationContext::IsLong() const { return m_trend.IsRising(); }

Qty mk::PositionOperationContext::GetPlannedQty() const {
  return m_settings.qty;
}

bool mk::PositionOperationContext::HasCloseSignal(
    const Position &position) const {
  return m_trend.IsRising() != position.IsLong() || !m_trend.IsExistent();
}

void mk::PositionOperationContext::SetCloseSignalPrice(const Price &price) {
  AssertLt(0, price);
  if (m_closeSignalPrice != 0) {
    return;
  }
  m_closeSignalPrice = price;
}

const Price &mk::PositionOperationContext::GetOpenSignalPrice() const {
  AssertNe(0, m_openSignalPrice);
  return m_openSignalPrice;
}
const Price &mk::PositionOperationContext::GetCloseSignalPrice() const {
  AssertNe(0, m_closeSignalPrice);
  return m_closeSignalPrice;
}

void mk::PositionOperationContext::OnCloseReasonChange(
    const CloseReason &, const CloseReason &newReason) {
  if (newReason != CLOSE_REASON_SIGNAL) {
    m_closeSignalPrice = 0;
  }
}

boost::shared_ptr<trdk::PositionOperationContext>
mk::PositionOperationContext::StartInvertedPosition(const Position &position) {
  Assert(HasCloseSignal(position));
  UseUnused(position);
  if (m_closeSignalPrice == 0) {
    return nullptr;
  }
  return boost::make_shared<PositionOperationContext>(m_settings, m_trend,
                                                      m_closeSignalPrice);
}
