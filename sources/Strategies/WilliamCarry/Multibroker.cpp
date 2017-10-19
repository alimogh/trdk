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
#include "Multibroker.hpp"
#include "TradingLib/PositionController.hpp"
#include "OperationContext.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::WilliamCarry;

namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

class Multibroker::Implementation : private boost::noncopyable {
 public:
  PositionController m_controller;

  explicit Implementation(Multibroker &self) : m_controller(self) {}
};

Multibroker::Multibroker(Context &context,
                         const std::string &instanceName,
                         const IniSectionRef &conf)
    : Base(context,
           "{C66325BA-B75C-4D3D-9E18-AB2AD4BD1916}",
           "WilliamCarryMultibroker",
           instanceName,
           conf),
      m_pimpl(boost::make_unique<Implementation>(*this)) {}

Multibroker::~Multibroker() = default;

void Multibroker::OpenPosition(OperationContext &&operation,
                               Security &security,
                               const Milestones &delayMeasurement) {
  if (!GetPositions().IsEmpty()) {
    throw Exception(
        "Failed to start new position as current strategy instance already "
        "handles position");
  }
  m_pimpl->m_controller.OnSignal(
      boost::make_shared<OperationContext>(std::move(operation)), security,
      delayMeasurement);
}

void Multibroker::OnLevel1Tick(Security &,
                               const pt::ptime &,
                               const Level1TickValue &,
                               const Milestones &) {}

void Multibroker::OnPositionUpdate(Position &position) {
  m_pimpl->m_controller.OnPositionUpdate(position);
}

void Multibroker::OnPostionsCloseRequest() {
  m_pimpl->m_controller.OnPostionsCloseRequest();
}
////////////////////////////////////////////////////////////////////////////////

boost::shared_ptr<Strategy> CreateMultibroker(Context &context,
                                              const std::string &instanceName,
                                              const IniSectionRef &conf) {
  return boost::make_shared<Multibroker>(context, instanceName, conf);
}
