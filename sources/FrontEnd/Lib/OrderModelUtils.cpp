/*******************************************************************************
 *   Created: 2018/01/24 16:24:43
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OrderModelUtils.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Detail;

namespace ids = boost::uuids;
namespace pt = boost::posix_time;

OrderRecord::OrderRecord(const Orm::Order &order, QString &&additionalInfo)
    : id(order.getRemoteId()),
      operationId(order.getOperation()->getId().toString()),
      subOperationId(order.getSubOperationId()),
      orderTime(ConvertFromDbDateTime(order.getOrderTime()).time()),
      symbol(order.getSymbol()),
      currency(order.getCurrency()),
      tradingSystem(order.getTradingSystem()),
      side(order.getIsBuy() ? ORDER_SIDE_BUY : ORDER_SIDE_SELL),
      sideName(ConvertToUiString(side)),
      qty(order.getQty()),
      price(order.getPrice()),
      tif(TIME_IN_FORCE_GTC),
      additionalInfo(std::move(additionalInfo)) {
  Update(order);
}

void OrderRecord::Update(const Orm::Order &order) {
  updateTime = order.getUpdateTime().time();
  status = static_cast<OrderStatus>(order.getStatus());
  statusName = ConvertToUiString(status);
  remainingQty = order.getRemainingQty();
  AssertGe(qty, remainingQty);
  filledQty = qty - remainingQty;
}
