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
  explicit SubOperationContext(
      TradingSystem &tradingSystem,
      const boost::shared_ptr<const TakeProfitStopLimit::Params>
          &takeProfitStopLimit,
      const std::vector<std::pair<boost::shared_ptr<const StopLoss::Params>,
                                  pt::time_duration>> &stopLosses)
      : m_orderPolicy(boost::make_shared<wc::OrderPolicy::Base>()),
        m_tradingSystem(tradingSystem),
        m_takeProfitStopLimit(takeProfitStopLimit),
        m_stopLosses(stopLosses) {}

  virtual ~SubOperationContext() override = default;

 public:
  virtual trdk::TradingSystem &GetTradingSystem(Strategy &,
                                                Security &) override {
    return m_tradingSystem;
  }

  virtual const tl::OrderPolicy &GetOpenOrderPolicy() const override {
    throw LogicError(
        "Mutibroker operation context doesn't provide open operation policy");
  }
  virtual const tl::OrderPolicy &GetCloseOrderPolicy() const override {
    return *m_orderPolicy;
  }

  virtual void Setup(Position &position) const override {
    for (const auto &params : m_stopLosses) {
      position.AttachAlgo(std::make_unique<StopLoss>(
          params.first, params.second, position, m_orderPolicy));
    }
    position.AttachAlgo(std::make_unique<TakeProfitStopLimit>(
        m_takeProfitStopLimit, position, m_orderPolicy));
  }

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

 private:
  boost::shared_ptr<tl::OrderPolicy> m_orderPolicy;
  TradingSystem &m_tradingSystem;

  std::vector<
      std::pair<boost::shared_ptr<const StopLoss::Params>, pt::time_duration>>
      m_stopLosses;
  boost::shared_ptr<const TakeProfitStopLimit::Params> m_takeProfitStopLimit;
};
}
////////////////////////////////////////////////////////////////////////////////

class OperationContext::Implementation : private boost::noncopyable {
 public:
  const Qty m_qty;
  boost::shared_ptr<OrderPolicy> m_orderPolicy;
  TradingSystem &m_tradingSystem;

  std::vector<
      std::pair<boost::shared_ptr<const StopLoss::Params>, pt::time_duration>>
      m_stopLosses;
  std::vector<boost::shared_ptr<const TakeProfitStopLimit::Params>>
      m_takeProfitStopLimits;

  explicit Implementation(const Qty &qty,
                          const Price &price,
                          TradingSystem &tradingSystem)
      : m_qty(qty),
        m_orderPolicy(boost::make_shared<OrderPolicy>(price)),
        m_tradingSystem(tradingSystem) {}
};

OperationContext::OperationContext(const Qty &qty,
                                   const Price &price,
                                   TradingSystem &tradingSystem)
    : m_pimpl(boost::make_unique<Implementation>(qty, price, tradingSystem)) {}

OperationContext::OperationContext(OperationContext &&) = default;

OperationContext::~OperationContext() = default;

const tl::OrderPolicy &OperationContext::GetOpenOrderPolicy() const {
  return *m_pimpl->m_orderPolicy;
}

const tl::OrderPolicy &OperationContext::GetCloseOrderPolicy() const {
  return *m_pimpl->m_orderPolicy;
}

void OperationContext::Setup(Position &position) const {
  for (const auto &params : m_pimpl->m_stopLosses) {
    position.AttachAlgo(std::make_unique<StopLoss>(
        params.first, params.second, position, m_pimpl->m_orderPolicy));
  }
}

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

bool OperationContext::HasSubOperations() const {
  return m_pimpl->m_takeProfitStopLimits.size() > 1;
}
std::vector<boost::shared_ptr<PositionOperationContext>>
OperationContext::StartSubOperations() {
  std::vector<boost::shared_ptr<PositionOperationContext>> result;
  result.reserve(m_pimpl->m_takeProfitStopLimits.size());
  for (const auto &stopLimit : m_pimpl->m_takeProfitStopLimits) {
    result.emplace_back(boost::make_shared<SubOperationContext>(
        m_pimpl->m_tradingSystem, stopLimit, m_pimpl->m_stopLosses));
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
