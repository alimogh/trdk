/*******************************************************************************
 *   Created: 2017/09/07 14:01:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OrderPolicy.hpp"
#include "Core/Position.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

////////////////////////////////////////////////////////////////////////////////

Price LimitOrderPolicy::GetOpenOrderPrice(Position &position) const {
  return position.GetMarketOpenPrice();
}
Price LimitOrderPolicy::GetCloseOrderPrice(Position &position) const {
  return position.GetMarketClosePrice();
}

////////////////////////////////////////////////////////////////////////////////

void LimitGtcOrderPolicy::Open(Position &position) const {
  position.Open(position.GetMarketOpenPrice());
}
void LimitGtcOrderPolicy::Close(Position &position) const {
  position.Close(position.GetMarketClosePrice());
}

////////////////////////////////////////////////////////////////////////////////

void LimitIocOrderPolicy::Open(Position &position) const {
  position.OpenImmediatelyOrCancel(GetOpenOrderPrice(position));
}
void LimitIocOrderPolicy::Close(Position &position) const {
  position.CloseImmediatelyOrCancel(GetCloseOrderPrice(position));
}

////////////////////////////////////////////////////////////////////////////////

void MarketGtcOrderPolicy::Open(Position &position) const {
  position.OpenAtMarketPrice();
}
void MarketGtcOrderPolicy::Close(Position &position) const {
  position.CloseAtMarketPrice();
}

////////////////////////////////////////////////////////////////////////////////
