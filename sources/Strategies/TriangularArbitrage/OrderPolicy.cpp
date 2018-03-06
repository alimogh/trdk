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
using namespace trdk::Strategies::TriangularArbitrage;

Price OrderPolicy::GetOpenOrderPrice(Position &position) const {
  const auto &pip = position.GetSecurity().GetPip();
  return Base::GetOpenOrderPrice(position) + (position.IsLong() ? pip : -pip);
}

Price OrderPolicy::GetCloseOrderPrice(Position &position) const {
  const auto &pip = position.GetSecurity().GetPip();
  return Base::GetCloseOrderPrice(position) + (position.IsLong() ? -pip : pip);
}
