//
//    Created: 2018/07/07 7:20 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "StrategyInstanceRecord.hpp"

using namespace trdk;
using namespace FrontEnd;
namespace ptr = boost::property_tree;
namespace json = ptr::json_parser;

class StrategyInstanceRecord::Implementation {
 public:
  QUuid m_id;
  QUuid m_typeId;
  bool m_isActive = false;
  QString m_name;
  std::vector<boost::weak_ptr<const OperationRecord>> m_operations;
  ptr::ptree m_config;

  Implementation() = default;
  Implementation(const QUuid &id, const QUuid &typeId)
      : m_id(id), m_typeId(typeId) {}
  Implementation(Implementation &&) = default;
  Implementation(const Implementation &) = default;
  Implementation &operator=(Implementation &&) = delete;
  Implementation &operator=(const Implementation &) = delete;
  ~Implementation() = default;
};

StrategyInstanceRecord::StrategyInstanceRecord()
    : m_pimpl(boost::make_unique<Implementation>()) {}
StrategyInstanceRecord::StrategyInstanceRecord(const QUuid &id,
                                               const QUuid &typeId)
    : m_pimpl(boost::make_unique<Implementation>(id, typeId)) {}
StrategyInstanceRecord::StrategyInstanceRecord(
    StrategyInstanceRecord &&) noexcept = default;
StrategyInstanceRecord::StrategyInstanceRecord(
    const StrategyInstanceRecord &rhs)
    : m_pimpl(boost::make_unique<Implementation>(*rhs.m_pimpl)) {}
StrategyInstanceRecord &StrategyInstanceRecord::operator=(
    StrategyInstanceRecord &&) noexcept = default;
StrategyInstanceRecord &StrategyInstanceRecord::operator=(
    const StrategyInstanceRecord &rhs) {
  StrategyInstanceRecord(rhs).Swap(*this);
  return *this;
}
StrategyInstanceRecord::~StrategyInstanceRecord() = default;
void StrategyInstanceRecord::Swap(StrategyInstanceRecord &rhs) {
  m_pimpl.swap(rhs.m_pimpl);
}

const QUuid &StrategyInstanceRecord::GetId() const { return m_pimpl->m_id; }
void StrategyInstanceRecord::SetIdValue(const QUuid &id) { m_pimpl->m_id = id; }

const QUuid &StrategyInstanceRecord::GetTypeId() const {
  return m_pimpl->m_typeId;
}
void StrategyInstanceRecord::SetTypeIdValue(const QUuid &typeId) {
  m_pimpl->m_typeId = typeId;
}

bool StrategyInstanceRecord::IsActive() const { return m_pimpl->m_isActive; }
void StrategyInstanceRecord::SetActive(const bool isActive) {
  m_pimpl->m_isActive = isActive;
}

const QString &StrategyInstanceRecord::GetName() const {
  return m_pimpl->m_name;
}
void StrategyInstanceRecord::SetName(QString name) {
  m_pimpl->m_name = std::move(name);
}

const std::vector<boost::weak_ptr<const OperationRecord>>
    &StrategyInstanceRecord::GetOperations() const {
  return m_pimpl->m_operations;
}
void StrategyInstanceRecord::SetOperationsValue(
    std::vector<boost::weak_ptr<const OperationRecord>> operations) {
  m_pimpl->m_operations = std::move(operations);
}
void StrategyInstanceRecord::AddOperation(
    const boost::shared_ptr<const OperationRecord> &operation) {
  m_pimpl->m_operations.emplace_back(operation);
}

const ptr::ptree &StrategyInstanceRecord::GetConfig() const {
  return m_pimpl->m_config;
}
void StrategyInstanceRecord::SetConfig(const ptr::ptree &config) {
  m_pimpl->m_config = config;
}
QString StrategyInstanceRecord::GetConfigValue() const {
  std::ostringstream stream;
  json::write_json(stream, m_pimpl->m_config, false);
  return QString::fromStdString(stream.str());
}
void StrategyInstanceRecord::SetConfigValue(const QString &configContent) {
  std::istringstream stream;
  stream.str(configContent.toStdString());
  ptr::ptree config;
  json::read_json(stream, config);
  SetConfig(std::move(config));
}
