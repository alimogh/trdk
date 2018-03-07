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
namespace Lib {
namespace Detail {

struct OperationRecord {
  const QString id;
  const QTime startTime;
  const QString strategyName;
  const QString strategyInstance;
  QString status;
  QTime endTime;
  boost::optional<Pnl::Result> result;
  QString financialResult;

 public:
  explicit OperationRecord(const boost::uuids::uuid &,
                           const boost::posix_time::ptime &,
                           const Strategy &);

 public:
  void Update(const Pnl::Data &);
  void Complete(const boost::posix_time::ptime &, const Pnl &);
};
}  // namespace Detail
}  // namespace Lib
}  // namespace FrontEnd
}  // namespace trdk