//
//    Created: 2018/06/21 10:13 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once
#ifndef TRDK_FRONTEND_LIB_INCLUDED_OPERATIONRECORD
#define TRDK_FRONTEND_LIB_INCLUDED_OPERATIONRECORD

#include "OperationStatus.hpp"
#ifdef ODB_COMPILER
#include "StrategyInstanceRecord.hpp"
#endif

namespace trdk {
namespace FrontEnd {

class OperationRecord {
#ifdef TRDK_FRONTEND_LIB
  friend class odb::access;
#endif
  OperationRecord();

 public:
  explicit OperationRecord(const QUuid &,
                           QDateTime startTime,
                           boost::shared_ptr<const StrategyInstanceRecord>,
                           const OperationStatus &);
  OperationRecord(OperationRecord &&) noexcept;
  OperationRecord(const OperationRecord &);
  OperationRecord &operator=(OperationRecord &&) noexcept;
  OperationRecord &operator=(const OperationRecord &);
  ~OperationRecord();
  void Swap(OperationRecord &) noexcept;

  const QUuid &GetId() const;

  const OperationStatus &GetStatus() const;
  void SetStatus(const OperationStatus &);

  const QDateTime &GetStartTime() const;
  const boost::optional<QDateTime> &GetEndTime() const;
  void SetEndTime(boost::optional<QDateTime>);

  const boost::shared_ptr<const StrategyInstanceRecord> &GetStrategyInstance()
      const;

  const std::vector<boost::weak_ptr<const OrderRecord>> &GetOrders() const;
  void AddOrder(boost::weak_ptr<const OrderRecord>);

  const std::vector<boost::weak_ptr<const PnlRecord>> &GetPnl() const;
  std::vector<boost::weak_ptr<const PnlRecord>> &GetPnl();

 private:
  void SetIdValue(const QUuid &);

  void SetStartTimeValue(QDateTime);

  void SetStrategyInstanceValue(
      boost::shared_ptr<const StrategyInstanceRecord>);

  void SetOrdersValue(std::vector<boost::weak_ptr<const OrderRecord>>);

  void SetPnlValue(std::vector<boost::weak_ptr<const PnlRecord>>);

#ifndef ODB_COMPILER

  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;

#else

#pragma db object(OperationRecord) table("operation")

#pragma db get(GetId) set(SetIdValue(std::move(?))) id
  QUuid id;
#pragma db get(GetStatus) set( \
    SetStatus(trdk::FrontEnd::OperationStatus::_from_integral(?))) not_null
  OperationStatus::_integral status;
#pragma db column("start_time") get(GetStartTime) \
    set(SetStartTimeValue(std::move(?))) not_null
  QDateTime startTime;
#pragma db column("end_time") get(GetEndTime) set(SetEndTime(std::move(?))) null
  boost::optional<QDateTime> endTime;
#pragma db column("strategy_instance") get(GetStrategyInstance) \
    set(SetStrategyInstanceValue(std::move(?))) value_not_null not_null
  boost::shared_ptr<const StrategyInstanceRecord> strategyInstance;
#pragma db get(GetOrders) set(SetOrdersValue(std::move(?))) inverse(operation) \
    value_not_null unordered
  std::vector<boost::weak_ptr<const OrderRecord>> orders;
#pragma db get(GetPnl) \
    set(SetPnlValue(std::move(?))) inverse(operation) value_not_null unordered
  std::vector<boost::weak_ptr<const PnlRecord>> pnl;

#pragma db index("operation_start_time_index") members(startTime)
#pragma db index("operation_end_time_index") members(endTime)
#pragma db index("operation_status_index") members(status)

#endif
};

}  // namespace FrontEnd
}  // namespace trdk

#ifdef ODB_COMPILER
#include "OrderRecord.hpp"
#include "PnlRecord.hpp"
#include "StrategyInstanceRecord.hpp"
#endif

#endif