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
#include "OrderPolicy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::WilliamCarry;

namespace pt = boost::posix_time;
namespace wc = trdk::Strategies::WilliamCarry;
namespace tl = trdk::TradingLib;

////////////////////////////////////////////////////////////////////////////////

namespace {
class SubOperationContext : public PositionOperationContext {
 public:
  explicit SubOperationContext(TradingSystem &tradingSystem,
                               const Price &closePrice)
      : m_stopLossOrderPolicy(closePrice),
        m_passiveOrderPolicy(closePrice),
        m_tradingSystem(tradingSystem) {}

  virtual ~SubOperationContext() override = default;

 public:
  virtual trdk::TradingSystem &GetTradingSystem(Strategy &,
                                                Security &) override {
    return m_tradingSystem;
  }

  virtual const tl::OrderPolicy &GetOpenOrderPolicy(
      const Position &) const override {
    throw LogicError(
        "Mutibroker operation context doesn't provide open operation policy");
  }
  virtual const tl::OrderPolicy &GetCloseOrderPolicy(
      const Position &position) const override {
    switch (position.GetCloseReason()) {
      case CLOSE_REASON_STOP_LOSS:
        return m_stopLossOrderPolicy;
      default:
        if (IsPassive(position.GetCloseReason())) {
          return m_passiveOrderPolicy;
        } else {
          return m_aggressiveOrderPolicy;
        }
    }
  }

  virtual void Setup(Position &) const override {}

  virtual bool IsLong(const Security &) const override {
    throw LogicError(
        "Mutibroker operation context doesn't provide operation side");
  }

  virtual Qty GetPlannedQty() const override {
    throw LogicError(
        "Mutibroker operation context doesn't provide operation size");
  }

  virtual bool HasCloseSignal(const Position &) const override { return false; }

  virtual boost::shared_ptr<PositionOperationContext> StartInvertedPosition(
      const Position &) override {
    return nullptr;
  }

  bool OnCloseReasonChange(Position &position,
                           const CloseReason &newReason) override {
    if (!position.HasActiveCloseOrders()) {
      return PositionOperationContext::OnCloseReasonChange(position, newReason);
    }
    if (IsPassive(position.GetCloseReason()) != IsPassive(newReason)) {
      position.CancelAllOrders();
      return false;
    }
    return PositionOperationContext::OnCloseReasonChange(position, newReason);
  }

 private:
  wc::StopLossLimitOrderPolicy m_stopLossOrderPolicy;
  wc::StopLimitOrderPolicy m_passiveOrderPolicy;
  LimitIocOrderPolicy m_aggressiveOrderPolicy;
  TradingSystem &m_tradingSystem;
};
}
////////////////////////////////////////////////////////////////////////////////

class OperationContext::Implementation : private boost::noncopyable {
 public:
  const Qty m_qty;
  OpenOrderPolicy m_orderPolicy;
  TradingSystem &m_tradingSystem;

  std::vector<
      std::pair<boost::shared_ptr<const StopLoss::Params>, pt::time_duration>>
      m_stopLosses;
  std::vector<boost::shared_ptr<const TakeProfitStopLimit::Params>>
      m_takeProfitStopLimits;

  explicit Implementation(const Qty &qty,
                          const Price &price,
                          TradingSystem &tradingSystem)
      : m_qty(qty), m_orderPolicy(price), m_tradingSystem(tradingSystem) {}
};

OperationContext::OperationContext(const Qty &qty,
                                   const Price &price,
                                   TradingSystem &tradingSystem)
    : m_pimpl(boost::make_unique<Implementation>(qty, price, tradingSystem)) {}

OperationContext::OperationContext(OperationContext &&) = default;

OperationContext::~OperationContext() = default;

const tl::OrderPolicy &OperationContext::GetOpenOrderPolicy(
    const Position &) const {
  return m_pimpl->m_orderPolicy;
}

const tl::OrderPolicy &OperationContext::GetCloseOrderPolicy(
    const Position &) const {
  return m_pimpl->m_orderPolicy;
}

void OperationContext::Setup(Position &) const {}

bool OperationContext::IsLong(const Security &) const {
  throw LogicError(
      "Mutibroker operation context doesn't provide operation side");
}

Qty OperationContext::GetPlannedQty() const { return m_pimpl->m_qty; }

bool OperationContext::HasCloseSignal(const Position &position) const {
  return position.GetOpenedQty() == 0 &&
         position.GetOpenStartTime() + pt::seconds(1) <=
             position.GetStrategy().GetContext().GetCurrentTime();
}

boost::shared_ptr<PositionOperationContext>
OperationContext::StartInvertedPosition(const Position &) {
  return nullptr;
}

void OperationContext::AddTakeProfitStopLimit(
    const Price &maxPriceChange, const pt::time_duration &activationTime) {
  m_pimpl->m_takeProfitStopLimits.emplace_back(
      boost::make_shared<TakeProfitStopLimit::Params>(maxPriceChange,
                                                      activationTime));
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

TradingSystem &OperationContext::GetTradingSystem(Strategy &, Security &) {
  return m_pimpl->m_tradingSystem;
}
const TradingSystem &OperationContext::GetTradingSystem(
    const Strategy &, const Security &) const {
  return m_pimpl->m_tradingSystem;
}

bool OperationContext::HasSubOperations() const {
  return !m_pimpl->m_takeProfitStopLimits.empty();
}
std::pair<std::vector<boost::shared_ptr<PositionOperationContext>>,
          std::vector<boost::shared_ptr<PositionOperationContext>>>
OperationContext::StartSubOperations(Position &rootOperation) {
  std::pair<std::vector<boost::shared_ptr<PositionOperationContext>>,
            std::vector<boost::shared_ptr<PositionOperationContext>>>
      result;

  const auto &startPrice = rootOperation.GetOpenAvgPrice();

  result.first.reserve(m_pimpl->m_takeProfitStopLimits.size());
  for (const auto &stopLimit : m_pimpl->m_takeProfitStopLimits) {
    auto priceDiff = stopLimit->GetMaxPriceOffsetPerLotToClose();
    if (!rootOperation.IsLong()) {
      priceDiff *= -1;
    }
    result.first.emplace_back(boost::make_shared<SubOperationContext>(
        m_pimpl->m_tradingSystem, startPrice + priceDiff));
  }

  result.second.reserve(m_pimpl->m_stopLosses.size());
  for (const auto &stopLimit : m_pimpl->m_stopLosses) {
    auto priceDiff = stopLimit.first->GetMaxLossPerLot();
    if (rootOperation.IsLong()) {
      priceDiff *= -1;
    }
    result.second.emplace_back(boost::make_shared<SubOperationContext>(
        m_pimpl->m_tradingSystem, startPrice + priceDiff));
  }

  return result;
}

////////////////////////////////////////////////////////////////////////////////
