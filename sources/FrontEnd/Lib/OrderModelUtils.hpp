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
namespace Detail {

struct OrderRecord {
  const QString id;
  const QTime orderTime;
  QString operationId;
  boost::optional<int64_t> subOperationId;
  const QString symbol;
  const QString currency;
  const QString tradingSystem;
  const OrderSide side;
  const QString sideName;
  const Qty qty;
  const Price price;
  const QString tif;
  OrderStatus status;
  QString statusName;
  Qty filledQty;
  Qty remainingQty;
  QTime updateTime;
  QString additionalInfo;

  explicit OrderRecord(const Orm::Order &, QString &&additionalInfo);

  void Update(const Orm::Order &);
};
}  // namespace Detail
}  // namespace FrontEnd
}  // namespace trdk
