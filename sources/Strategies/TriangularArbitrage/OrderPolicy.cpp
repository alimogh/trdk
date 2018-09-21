/*******************************************************************************
 *   Created: 2018/03/10 03:53:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OrderPolicy.hpp"

using namespace trdk;
using namespace Strategies::TriangularArbitrage;

Price OrderPolicy::GetOpenOrderPrice(Position &position) const {
  // Also see GetOrderQtyAllowedByBalance method.
  return Base::GetOpenOrderPrice(position) * (position.IsLong() ? 1.03 : 0.97);
}

Price OrderPolicy::GetCloseOrderPrice(Position &position) const {
  // Also see GetOrderQtyAllowedByBalance method.
  return Base::GetCloseOrderPrice(position) + (position.IsLong() ? 0.97 : 1.03);
}
