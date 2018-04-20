/*******************************************************************************
 *   Created: 2018/01/27 16:06:01
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

struct OperationRecord {
  const QString id;
  const QDateTime startTime;
  const QString strategyName;
  const QString strategyParams;
  QString statusName;
  QDateTime endTime;
  Orm::OperationStatus::enum_OperationStatus status;
  QString financialResult;
  QString commission;
  QString totalResult;

  explicit OperationRecord(const Orm::Operation &);

  void Update(const Orm::Operation &);
};
}  // namespace Detail
}  // namespace FrontEnd
}  // namespace trdk