/*******************************************************************************
 *   Created: 2017/11/10 17:19:48
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
using namespace trdk::Lib;
using namespace trdk::Strategies::ArbitrageAdvisor;

////////////////////////////////////////////////////////////////////////////////

OpenOrderPolicy::OpenOrderPolicy(const Price &sellPrice, const Price &buyPrice)
    : m_sellPrice(sellPrice), m_buyPrice(buyPrice) {}

Price OpenOrderPolicy::GetOpenOrderPrice(bool isLong) const {
  return isLong ? m_buyPrice : m_sellPrice;
}

Price OpenOrderPolicy::GetOpenOrderPrice(Position &position) const {
  return GetOpenOrderPrice(position.IsLong());
}

Price OpenOrderPolicy::GetCloseOrderPrice(Position &) const {
  throw LogicError("Order policy doesn't allow close positions");
}

////////////////////////////////////////////////////////////////////////////////

Price CloseOrderPolicy::GetOpenOrderPrice(Position &) const {
  throw LogicError("Order policy doesn't allow open positions");
}

Price CloseOrderPolicy::GetCloseOrderPrice(Position &position) const {
  return Base::GetCloseOrderPrice(position) * (position.IsLong() ? 0.95 : 1.05);
}

////////////////////////////////////////////////////////////////////////////////
