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
#include "TradingLib/PositionController.hpp"
#include "OperationContext.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::WilliamCarry;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

class MultibrokerStrategy::Implementation : private boost::noncopyable {
 public:
  std::vector<boost::shared_ptr<PositionController>> m_controllers;

  explicit Implementation(MultibrokerStrategy &) {}
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
    const Milestones &delayMeasurement) {
  if (!GetPositions().IsEmpty()) {
    throw Exception(
        "Failed to start new position as current strategy instance already "
        "handles position");
  }
  AssertEq(0, m_pimpl->m_controllers.size());
  m_pimpl->m_controllers.clear();
  m_pimpl->m_controllers.reserve(operations.size());
  for (size_t i = 0; i < std::min(GetContext().GetNumberOfTradingSystems(),
                                  operations.size());
       ++i) {
    m_pimpl->m_controllers.emplace_back(boost::make_shared<PositionController>(
        *this, GetContext().GetTradingSystem(i, GetTradingMode())));
    m_pimpl->m_controllers.back()->OpenPosition(
        boost::make_shared<OperationContext>(std::move(operations[i])),
        security, delayMeasurement);
  }
}

void MultibrokerStrategy::OnLevel1Tick(Security &,
                                       const pt::ptime &,
                                       const Level1TickValue &,
                                       const Milestones &) {}

void MultibrokerStrategy::OnPositionUpdate(Position &position) {
  AssertGt(m_pimpl->m_controllers.size(),
           position.GetTradingSystem().GetIndex());
  if (m_pimpl->m_controllers.size() <= position.GetTradingSystem().GetIndex()) {
    return;
  }
  m_pimpl->m_controllers[position.GetTradingSystem().GetIndex()]
      ->OnPositionUpdate(position);
}

void MultibrokerStrategy::OnPostionsCloseRequest() {
  for (auto &controller : m_pimpl->m_controllers) {
    controller->OnPostionsCloseRequest();
  }
}
////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<Strategy> CreateMultibroker(Context &context,
                                              const std::string &instanceName,
                                              const IniSectionRef &conf) {
  return boost::make_shared<MultibrokerStrategy>(context, instanceName, conf);
}

////////////////////////////////////////////////////////////////////////////////
