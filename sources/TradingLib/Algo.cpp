/*******************************************************************************
 *   Created: 2017/12/21 14:24:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Algo.hpp"

using namespace trdk;
using namespace trdk::TradingLib;

Algo::Algo(Position &position) : m_position(position) {}

Module::TradingLog &Algo::GetTradingLog() const noexcept {
  return GetPosition().GetStrategy().GetTradingLog();
}
