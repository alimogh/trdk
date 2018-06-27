/*******************************************************************************
 *   Created: 2018/02/04 23:07:38
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Controller.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::PingPong;

Controller::Controller(bool isLongOpeningEnabled,
                       bool isShortOpeningEnabled,
                       bool isClosingEnabled)
    : m_isLongOpeningEnabled(isLongOpeningEnabled),
      m_isShortOpeningEnabled(isShortOpeningEnabled),
      m_isClosingEnabled(isClosingEnabled) {}

bool Controller::IsLongOpeningEnabled() const { return m_isLongOpeningEnabled; }
bool Controller::IsShortOpeningEnabled() const {
  return m_isShortOpeningEnabled;
}
void Controller::EnableLongOpening(bool isEnabled) {
  m_isLongOpeningEnabled = isEnabled;
}
void Controller::EnableShortOpening(bool isEnabled) {
  m_isShortOpeningEnabled = isEnabled;
}

bool Controller::IsClosingEnabled() const { return m_isClosingEnabled; }
void Controller::EnableClosing(bool isEnabled) {
  m_isClosingEnabled = isEnabled;
}

Position *Controller::Open(const boost::shared_ptr<Operation> &operation,
                           int64_t subOperationId,
                           Security &security,
                           bool isLong,
                           const Qty &qty,
                           const Milestones &delayMeasurement) {
  if ((operation->IsLong(security) && !m_isLongOpeningEnabled) ||
      (!operation->IsLong(security) && !m_isShortOpeningEnabled)) {
    return nullptr;
  }
  {
    const auto &checkError =
        OrderBestSecurityChecker::Create(operation->GetStrategy(), isLong, qty)
            ->Check(security);
    if (checkError) {
      operation->GetStrategy().GetLog().Warn(
          "%1% is not suitable target to open %2%-position with qty %3%: "
          "\"%4%\"",
          security,                       // 1
          isLong ? "long" : "short",      // 2
          qty,                            // 3
          checkError->GetRuleNameRef());  // 4
      const auto &tradingSystem =
          operation->GetStrategy()
              .GetTradingSystem(security.GetSource().GetIndex())
              .GetTitle();
      boost::polymorphic_downcast<Strategy *>(&operation->GetStrategy())
          ->RaiseEvent("Got signal from " + tradingSystem + " to open \"" +
                       std::string(isLong ? "long" : "short") +
                       "\", but can't open position: " +
                       checkError->GetRuleNameRef() + ".");
      return nullptr;
    }
  }
  return Base::Open(operation, subOperationId, security, isLong, qty,
                    delayMeasurement);
}

bool Controller::Close(Position &position, const CloseReason &reason) {
  if (!m_isClosingEnabled) {
    return false;
  }
  return Base::Close(position, reason);
}

void Controller::Close(Position &position) {
  if (!m_isClosingEnabled) {
    return;
  }
  {
    const auto &checkError =
        PositionBestSecurityChecker::Create(position)->Check(
            position.GetSecurity());
    if (checkError) {
      position.GetStrategy().GetLog().Error(
          "%1% is not suitable target to close position %2%/%3%: \"%4%\"",
          position.GetSecurity(),            // 1
          position.GetOperation()->GetId(),  // 2
          position.GetSubOperationId(),      // 3
          checkError->GetRuleNameRef());     // 4
      position.MarkAsCompleted();
      boost::polymorphic_downcast<Strategy *>(&position.GetStrategy())
          ->RaiseEvent("Got signal from " +
                       position.GetTradingSystem().GetTitle() + " to close \"" +
                       std::string(position.IsLong() ? "long" : "short") +
                       "\", but can't close position: " +
                       checkError->GetRuleNameRef() + ".");
      return;
    }
  }
  Base::Close(position);
}

Position *Controller::GetExisting(trdk::Strategy &strategy,
                                  Security &security) {
  for (auto &position : strategy.GetPositions()) {
    if (&position.GetSecurity() == &security) {
      return &position;
    }
  }
  return nullptr;
}

bool Controller::HasPositions(trdk::Strategy &strategy,
                              Security &security) const {
  return const_cast<Controller *>(this)->GetExisting(strategy, security)
             ? true
             : false;
}
