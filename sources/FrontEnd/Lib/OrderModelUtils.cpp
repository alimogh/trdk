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
using namespace trdk::FrontEnd::Lib::Detail;

namespace ids = boost::uuids;
namespace pt = boost::posix_time;

OrderRecord::OrderRecord(const QString &id,
                         const boost::optional<ids::uuid> &operationId,
                         const boost::optional<int64_t> &subOperationId,
                         const pt::ptime &time,
                         const Security &security,
                         const Currency &currency,
                         const TradingSystem &tradingSystem,
                         const OrderSide &side,
                         const Qty &qty,
                         const boost::optional<Price> &price,
                         const OrderStatus &status,
                         const TimeInForce &tif,
                         const QString &additionalInfo)
    : id(id),
      operationId(operationId
                      ? QString::fromStdString(
                            boost::lexical_cast<std::string>(*operationId))
                      : ""),
      subOperationId(subOperationId),
      orderTime(ConvertToQDateTime(time).time()),
      symbol(QString::fromStdString(security.GetSymbol().GetSymbol())),
      currency(QString::fromStdString(ConvertToIso(currency))),
      exchangeName(QString::fromStdString(tradingSystem.GetInstanceName())),
      side(side),
      sideName(ConvertToUiString(side)),
      qty(qty),
      price(boost::get_optional_value_or(
          price, std::numeric_limits<double>::quiet_NaN())),
      tif(ConvertToUiString(tif)),
      status(status),
      statusName(ConvertToUiString(status)),
      filledQty(0),
      remainingQty(qty),
      additionalInfo(additionalInfo) {
  AssertGe(qty, remainingQty);
}

void OrderRecord::Update(const pt::ptime &time,
                         const OrderStatus &newStatus,
                         const Qty &newRemainingQty) {
  lastTime = ConvertToQDateTime(time).time();
  status = newStatus;
  statusName = ConvertToUiString(status);
  remainingQty = newRemainingQty;
  AssertGe(qty, remainingQty);
  filledQty = qty - remainingQty;
}
