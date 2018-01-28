/*******************************************************************************
 *   Created: 2018/01/16 15:37:31
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"

using namespace trdk;
using namespace trdk::Strategies;

namespace aa = trdk::Strategies::ArbitrageAdvisor;

Position *aa::FindOppositePosition(Position &position) {
  Position *result = nullptr;
  for (auto &oppositePosition : position.GetStrategy().GetPositions()) {
    if (oppositePosition.GetOperation() == position.GetOperation() &&
        &oppositePosition != &position) {
      Assert(!result);
      result = &oppositePosition;
#ifndef BOOST_ENABLE_ASSERT_HANDLER
      break;
#endif
    }
  }
  return result;
}

const Position *aa::FindOppositePosition(const Position &position) {
  return FindOppositePosition(const_cast<Position &>(position));
}
