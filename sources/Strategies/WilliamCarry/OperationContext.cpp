/*******************************************************************************
 *   Created: 2017/10/14 00:10:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OperationContext.hpp"
#include "TradingLib/OrderPolicy.hpp"
#include "TradingLib/StopLimit.hpp"
#include "TradingLib/StopLoss.hpp"
#include "Core/Position.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::WilliamCarry;

namespace pt = boost::posix_time;

class OperationContext::Implementation : private boost::noncopyable {
 public:
  const bool m_isLong;
  const Qty &m_qty;
  boost::shared_ptr<OrderPolicy> m_orderPolicy;

  std::vector<
      std::pair<boost::shared_ptr<const StopLoss::Params>, pt::time_duration>>
      m_stopLosses;
  std::vector<boost::shared_ptr<const TakeProfitStopLimit::Params>>
      m_takeProfitStopLimits;
  pt::time_duration m_stopTime;

  explicit Implementation(bool isLong, const Qty &qty)
      : m_isLong(isLong),
        m_qty(qty),
        m_orderPolicy(boost::make_shared<LimitIocOrderPolicy>()) {}
};

OperationContext::OperationContext(bool isLong, const Qty &qty)
    : m_pimpl(boost::make_unique<Implementation>(isLong, qty)) {}

OperationContext::OperationContext(OperationContext &&) = default;

OperationContext::~OperationContext() = default;

const OrderPolicy &OperationContext::GetOpenOrderPolicy() const {
  return *m_pimpl->m_orderPolicy;
}

const OrderPolicy &OperationContext::GetCloseOrderPolicy() const {
  return *m_pimpl->m_orderPolicy;
}

void OperationContext::Setup(Position &position) const {
  for (const auto &params : m_pimpl->m_stopLosses) {
    position.AttachAlgo(std::make_unique<StopLoss>(params.first, position,
                                                   m_pimpl->m_orderPolicy));
  }
  for (const auto &params : m_pimpl->m_takeProfitStopLimits) {
    position.AttachAlgo(std::make_unique<TakeProfitStopLimit>(
        params, position, m_pimpl->m_orderPolicy));
  }
}

bool OperationContext::IsLong() const { return m_pimpl->m_isLong; }

Qty OperationContext::GetPlannedQty() const { return m_pimpl->m_qty; }

bool OperationContext::HasCloseSignal(const Position &position) const {
  return position.GetOpenedQty() == 0;
}

boost::shared_ptr<PositionOperationContext>
OperationContext::StartInvertedPosition(const Position &) {
  return nullptr;
}

void OperationContext::AddTakeProfitStopLimit(
    const Price &maxPriceChange,
    const pt::time_duration &activationTime,
    const Double &volumeToCloseRatio) {
  m_pimpl->m_takeProfitStopLimits.emplace_back(
      boost::make_shared<TakeProfitStopLimit::Params>(
          maxPriceChange, activationTime, volumeToCloseRatio));
}

void OperationContext::AddStopLoss(const Price &maxPriceChange) {
  m_pimpl->m_stopLosses.emplace_back(
      boost::make_shared<StopLoss::Params>(maxPriceChange), pt::seconds(0));
}

void OperationContext::AddStopLoss(const Price &maxPriceChange,
                                   const pt::time_duration &startDelay) {
  m_pimpl->m_stopLosses.emplace_back(
      boost::make_shared<StopLoss::Params>(maxPriceChange), startDelay);
}
