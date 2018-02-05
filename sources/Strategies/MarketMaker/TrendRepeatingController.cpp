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
#include "TrendRepeatingController.hpp"
#include "TrendRepeatingStrategy.hpp"

using namespace trdk;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::MarketMaker;

TrendRepeatingController::TrendRepeatingController()
    : m_isClosingEnabled(true) {}

bool TrendRepeatingController::IsClosedEnabled() const {
  return m_isClosingEnabled;
}

void TrendRepeatingController::EnableClosing(bool isEnabled) {
  m_isClosingEnabled = isEnabled;
}

bool TrendRepeatingController::ClosePosition(Position &position,
                                             const CloseReason &reason) {
  if (!m_isClosingEnabled) {
    return false;
  }
  return Base::ClosePosition(position, reason);
}

void TrendRepeatingController::ClosePosition(Position &position) {
  if (!m_isClosingEnabled) {
    return;
  }
  {
    const auto &checker = BestSecurityCheckerForPosition::Create(position);
    auto &strategy = *boost::polymorphic_cast<TrendRepeatingStrategy *>(
        &position.GetStrategy());
    strategy.ForEachSecurity(
        [&checker](Security &security) { checker->Check(security); });
    if (!checker->HasSuitableSecurity()) {
      position.GetStrategy().GetLog().Error(
          "Failed to find suitable security for the position \"%1%/%2%\" "
          "(actual security is \"%3%\") to close the rest of the position "
          "%4$.8f out of %5$.8f.",
          position.GetOperation()->GetId(),  // 1
          position.GetSubOperationId(),      // 2
          position.GetSecurity(),            // 3
          position.GetOpenedQty(),           // 4
          position.GetActiveQty());          // 5
      position.MarkAsCompleted();
      strategy.RaiseEvent(
          "Failed to find suitable exchange for the position. Position marked "
          "as completed with unclosed rest.");
      return;
    }
  }
  Base::ClosePosition(position);
}
