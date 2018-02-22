/*******************************************************************************
 *   Created: 2018/01/24 09:38:11
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace FrontEnd {
namespace Lib {
namespace Detail {

struct OrderRecord {
  const QString id;
  const QTime orderTime;
  QString operationId;
  boost::optional<int64_t> subOperationId;
  const QString symbol;
  const QString currency;
  const QString exchangeName;
  const OrderSide side;
  const QString sideName;
  const double qty;
  const double price;
  const QString tif;
  OrderStatus status;
  QString statusName;
  Qty filledQty;
  Qty remainingQty;
  QTime lastTime;
  QString additionalInfo;

  explicit OrderRecord(const QString &id,
                       const boost::optional<boost::uuids::uuid> &operationId,
                       const boost::optional<int64_t> &subOperationId,
                       const boost::posix_time::ptime &,
                       const Security &,
                       const trdk::Lib::Currency &,
                       const TradingSystem &,
                       const OrderSide &,
                       const Qty &,
                       const boost::optional<Price> &,
                       const OrderStatus &,
                       const TimeInForce &,
                       const QString &additionalInfo);

  void Update(const boost::posix_time::ptime &,
              const OrderStatus &,
              const Qty &remainingQty);
};
}  // namespace Detail
}  // namespace Lib
}  // namespace FrontEnd
}  // namespace trdk
