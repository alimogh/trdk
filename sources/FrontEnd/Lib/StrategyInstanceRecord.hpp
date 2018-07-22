//
//    Created: 2018/06/28 19:06
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once
#ifndef TRDK_FRONTEND_LIB_INCLUDED_STRATEGYINSTANCERECORD
#define TRDK_FRONTEND_LIB_INCLUDED_STRATEGYINSTANCERECORD

namespace trdk {
namespace FrontEnd {

class StrategyInstanceRecord {
#ifdef TRDK_FRONTEND_LIB
  friend class odb::access;
#endif

  StrategyInstanceRecord();

 public:
  explicit StrategyInstanceRecord(const QUuid &id, const QUuid &typeId);
  StrategyInstanceRecord(StrategyInstanceRecord &&) noexcept;
  StrategyInstanceRecord(const StrategyInstanceRecord &);
  StrategyInstanceRecord &operator=(StrategyInstanceRecord &&) noexcept;
  StrategyInstanceRecord &operator=(const StrategyInstanceRecord &);
  ~StrategyInstanceRecord();
  void Swap(StrategyInstanceRecord &);

  const QUuid &GetId() const;
  const QUuid &GetTypeId() const;

  bool IsActive() const;
  void SetActive(bool);

  const QString &GetName() const;
  void SetName(QString);

  const std::vector<boost::weak_ptr<const OperationRecord>> &GetOperations()
      const;
  void AddOperation(const boost::shared_ptr<const OperationRecord> &);

  const boost::property_tree::ptree &GetConfig() const;
  void SetConfig(const boost::property_tree::ptree &);

 private:
  void SetIdValue(const QUuid &);
  void SetTypeIdValue(const QUuid &);
  void SetOperationsValue(std::vector<boost::weak_ptr<const OperationRecord>>);

  QString GetConfigValue() const;
  void SetConfigValue(const QString &);

#ifndef ODB_COMPILER

  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;

#else

#pragma db object(StrategyInstanceRecord) table("strategy_instance")

#pragma db get(GetId) set(SetIdValue(std::move(?))) id
  QUuid id;
#pragma db column("type_id") get(GetTypeId) \
    set(SetTypeIdValue(std::move(?))) not_null
  QUuid typeId;
#pragma db column("is_active") get(IsActive) set(SetActive) not_null
  bool isActive;
#pragma db get(GetName) set(SetName(std::move(?))) not_null
  QString name;
#pragma db get(GetOperations) set(SetOperationsValue(std::move(?))) \
    inverse(strategyInstance) value_not_null unordered
  std::vector<boost::weak_ptr<const OperationRecord>> operations;
#pragma db get(GetConfigValue) set(SetConfigValue) null
  QString config;

  //! @todo Temporary index, while we are searching strategy instance by name.
#pragma db index("name_index") members(name)

#endif
};
}  // namespace FrontEnd
}  // namespace trdk

#ifdef ODB_COMPILER
#include "OperationRecord.hpp"
#endif

#endif