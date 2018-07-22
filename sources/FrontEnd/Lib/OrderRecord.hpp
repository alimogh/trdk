//
//    Created: 2018/06/21 11:39
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once
#ifndef TRDK_FRONTEND_LIB_INCLUDED_ORDERRECORD
#define TRDK_FRONTEND_LIB_INCLUDED_ORDERRECORD

#ifdef ODB_COMPILER
#include "OperationRecord.hpp"
#endif

namespace trdk {
namespace FrontEnd {

class OrderRecord {
#ifdef TRDK_FRONTEND_LIB
  friend class odb::access;
#endif

 public:
  typedef uintmax_t Id;
  typedef int64_t SubOperationId;

 private:
  OrderRecord();

 public:
  explicit OrderRecord(QString remoteId,
                       QString symbol,
                       const Lib::Currency &,
                       QString tradingSystemInstanceName,
                       const OrderSide &,
                       const Qty &,
                       boost::optional<Price>,
                       const TimeInForce &,
                       QDateTime submitTime,
                       const OrderStatus &);
  explicit OrderRecord(QString remoteId,
                       const SubOperationId &,
                       boost::shared_ptr<const OperationRecord>,
                       QString symbol,
                       const Lib::Currency &,
                       QString tradingSystemInstanceName,
                       const OrderSide &,
                       const Qty &,
                       boost::optional<Price>,
                       const TimeInForce &,
                       QDateTime submitTime,
                       const OrderStatus &);
  OrderRecord(OrderRecord &&) noexcept;
  OrderRecord(const OrderRecord &);
  OrderRecord &operator=(OrderRecord &&) noexcept;
  OrderRecord &operator=(const OrderRecord &);
  ~OrderRecord();
  void Swap(OrderRecord &) noexcept;

  const Id &GetId() const;
  const QString &GetRemoteId() const;

  const boost::shared_ptr<const OperationRecord> &GetOperation() const;
  const boost::optional<SubOperationId> &GetSubOperationId() const;

  const QString &GetSymbol() const;

  const Lib::Currency &GetCurrency() const;

  const QString &GetTradingSystemInstanceName() const;

  const OrderSide &GetSide() const;

  const Qty &GetQty() const;

  const Qty &GetRemainingQty() const;
  void SetRemainingQty(const Qty &);

  const boost::optional<Price> &GetPrice() const;

  const TimeInForce &GetTimeInForce() const;

  const QDateTime &GetSubmitTime() const;

  const QDateTime &GetUpdateTime() const;
  void SetUpdateTime(QDateTime) const;

  const OrderStatus &GetStatus() const;
  void SetStatus(const OrderStatus &);

  const boost::optional<QString> &GetAdditionalInfo() const;
  void SetAdditionalInfo(boost::optional<QString>);

 private:
  void SetIdValue(const Id &);
  void SetRemoteIdValue(QString);

  void SetOperationValue(boost::shared_ptr<const OperationRecord>);
  void SetSubOperationIdValue(boost::optional<SubOperationId>);

  void SetSymbolValue(QString);

  void SetCurrencyValue(const Lib::Currency &);

  void SetTradingSystemInstanceNameValue(QString);

  void SetSideValue(const OrderSide &);

  void SetQtyValue(const Qty &);

  boost::optional<double> GetPriceValue() const;
  void SetPriceValue(const boost::optional<double> &);

  void SetTimeInForceValue(const TimeInForce &);

  void SetSubmitTimeValue(QDateTime);

#ifndef ODB_COMPILER

  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;

#else

#pragma db object(OrderRecord) table("order")

#pragma db get(GetId) set(SetIdValue) id auto
  Id id;
#pragma db column("remote_id") get(GetRemoteId) \
    set(SetRemoteIdValue(std::move(?))) not_null
  QString remoteId;
#pragma db get(GetOperation) \
    set(SetOperationValue(std::move(?))) value_not_null null
  boost::shared_ptr<const OperationRecord> operation;
#pragma db column("sub_operation_id") get(GetSubOperationId) \
    set(SetSubOperationIdValue(std::move(?))) null
  boost::optional<SubOperationId> subOperationId;
#pragma db get(GetSymbol) set(SetSymbolValue(std::move(?))) not_null
  QString symbol;
#pragma db get(GetCurrency) \
    set(SetCurrencyValue(trdk::Lib::Currency::_from_integral(?))) not_null
  Lib::Currency::_integral currency;
#pragma db column("trading_system_instance_name") \
    get(GetTradingSystemInstanceName)             \
        set(SetTradingSystemInstanceNameValue(std::move(?))) not_null
  QString tradingSystemInstance;
#pragma db get(GetSide) \
    set(SetSideValue(trdk::OrderSide::_from_integral(?))) not_null
  OrderSide::_integral side;
#pragma db get(GetQty) set(SetQtyValue) not_null
  Qty::ValueType qty;
#pragma db column("remaining_qty") get(GetRemainingQty) set(SetRemainingQty) \
    not_null
  Qty::ValueType remainingQty;
#pragma db get(GetPriceValue) set(SetPriceValue) null
  boost::optional<Price::ValueType> price;
#pragma db column("time_in_force") get(GetTimeInForce) \
    set(SetTimeInForceValue) not_null
  TimeInForce timeInForce;
#pragma db column("submit_time") get(GetSubmitTime) \
    set(SetSubmitTimeValue(std::move(?))) not_null
  QDateTime submitTime;
#pragma db column("update_time") get(GetUpdateTime) \
    set(SetUpdateTime(std::move(?))) not_null
  QDateTime updateTime;
#pragma db get(GetStatus) \
    set(SetStatus(trdk::OrderStatus::_from_integral(?))) not_null
  OrderStatus::_integral status;
#pragma db column("additional_info") get(GetAdditionalInfo) \
    set(SetAdditionalInfo(std::move(?))) null
  boost::optional<QString> additionalInfo;

#pragma db index("order_remote_id_index") \
    unique members(tradingSystemInstance, remoteId)
#pragma db index("order_status_index") members(status)

#endif
};

}  // namespace FrontEnd
}  // namespace trdk

#endif
