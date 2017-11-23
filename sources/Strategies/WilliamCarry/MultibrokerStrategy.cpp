/*******************************************************************************
 *   Created: 2017/10/12 11:27:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MultibrokerStrategy.hpp"
#include "OperationContext.hpp"
#include "PositionController.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::WilliamCarry;

namespace pt = boost::posix_time;
namespace sig = boost::signals2;

////////////////////////////////////////////////////////////////////////////////

class MultibrokerStrategy::Implementation : private boost::noncopyable {
 public:
  WilliamCarry::PositionController m_controller;
  sig::signal<void(
      bool isLong, const Security &, const TradingSystem &, bool isActive)>
      m_positionSignal;

  explicit Implementation(MultibrokerStrategy &self) : m_controller(self) {}
};

MultibrokerStrategy::MultibrokerStrategy(Context &context,
                                         const std::string &instanceName,
                                         const IniSectionRef &conf)
    : Base(context,
           "{C66325BA-B75C-4D3D-9E18-AB2AD4BD1916}",
           "WilliamCarryMultibroker",
           instanceName,
           conf),
      m_pimpl(boost::make_unique<Implementation>(*this)) {}

MultibrokerStrategy::~MultibrokerStrategy() = default;

void MultibrokerStrategy::OpenPosition(
    std::vector<OperationContext> &&operations,
    Security &security,
    bool isLong,
    const Milestones &delayMeasurement) {
  Assert(!operations.empty());
  if (!GetPositions().IsEmpty()) {
    if (GetPositions().cbegin()->IsLong() == isLong) {
      throw Exception(
          "Failed to start new position as current strategy instance already "
          "handles position");
    }
    m_pimpl->m_controller.OnPostionsCloseRequest();
  } else {
    for (OperationContext &operation : operations) {
      m_pimpl->m_controller.OpenPosition(
          boost::make_shared<OperationContext>(std::move(operation)), security,
          isLong, delayMeasurement);
    }
  }
}

void MultibrokerStrategy::OnLevel1Tick(Security &,
                                       const pt::ptime &,
                                       const Level1TickValue &,
                                       const Milestones &) {}

void MultibrokerStrategy::OnPositionUpdate(Position &position) {
  m_pimpl->m_controller.OnPositionUpdate(position);

  for (const auto &strategyPosition : GetPositions()) {
    if (!strategyPosition.IsCompleted()) {
      m_pimpl->m_positionSignal(position.IsLong(), position.GetSecurity(),
                                position.GetTradingSystem(), true);
      return;
    }
  }
  m_pimpl->m_positionSignal(position.IsLong(), position.GetSecurity(),
                            position.GetTradingSystem(),

                            false);
}

void MultibrokerStrategy::OnPostionsCloseRequest() {
  m_pimpl->m_controller.OnPostionsCloseRequest();
}

sig::scoped_connection MultibrokerStrategy::SubscribeToPositionsUpdates(
    const boost::function<void(
        bool isLong, const Security &, const TradingSystem &, bool isActive)>
        &slot) const {
  return m_pimpl->m_positionSignal.connect(slot);
}

////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<Strategy> CreateMultibroker(Context &context,
                                              const std::string &instanceName,
                                              const IniSectionRef &conf) {
  return boost::make_shared<MultibrokerStrategy>(context, instanceName, conf);
}

////////////////////////////////////////////////////////////////////////////////
