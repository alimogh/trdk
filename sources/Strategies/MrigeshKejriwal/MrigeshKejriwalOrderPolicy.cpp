/*******************************************************************************
 *   Created: 2017/09/11 09:39:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MrigeshKejriwalOrderPolicy.hpp"
#include "Core/Position.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::MrigeshKejriwal;

OrderPolicy::OrderPolicy(const Price &correction) : m_correction(correction) {}

ScaledPrice OrderPolicy::GetOpenOrderPrice(Position &position) const {
  return Base::GetOpenOrderPrice(position) +
         position.GetSecurity().ScalePrice(position.IsLong() ? m_correction
                                                             : -m_correction);
}

ScaledPrice OrderPolicy::GetCloseOrderPrice(Position &position) const {
  return Base::GetCloseOrderPrice(position) +
         position.GetSecurity().ScalePrice(position.IsLong() ? -m_correction
                                                             : m_correction);
}
