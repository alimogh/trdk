/*******************************************************************************
 *   Created: 2018/01/30 01:51:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "GeneralAlgos.hpp"

using namespace trdk;
using namespace TradingLib;
namespace tl = TradingLib;

boost::optional<OrderCheckError> tl::CheckPositionRestAsOrder(
    const Position &position) {
  return CheckPositionRestAsOrder(position, position.GetSecurity(),
                                  position.GetTradingSystem());
}

boost::optional<OrderCheckError> tl::CheckPositionRestAsOrder(
    const Position &position,
    const Security &security,
    const TradingSystem &tradingSystem) {
  Price price;
  OrderSide side;
  if (position.GetNumberOfCloseOrders() || position.IsFullyOpened()) {
    price = position.GetMarketClosePrice();
    side = position.GetCloseOrderSide();
  } else {
    price = position.GetMarketOpenPrice();
    side = position.GetOpenOrderSide();
  }
  return tradingSystem.CheckOrder(security, position.GetCurrency(),
                                  position.GetActiveQty(), price, side);
}