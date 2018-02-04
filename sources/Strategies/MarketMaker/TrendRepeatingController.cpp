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

using namespace trdk;
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
  Base::ClosePosition(position);
}
