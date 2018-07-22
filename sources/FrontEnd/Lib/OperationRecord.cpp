//
//    Created: 2018/07/07 11:58 AM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "OperationRecord.hpp"
#include "PnlRecord.hpp"

using namespace trdk;
using namespace FrontEnd;

class OperationRecord::Implementation {
 public:
  QUuid m_id;

  OperationStatus m_status = OperationStatus::Error;

  QDateTime m_startTime;
  boost::optional<QDateTime> m_endTime;

  boost::shared_ptr<const StrategyInstanceRecord> m_strategyInstance;

  std::vector<boost::weak_ptr<const OrderRecord>> m_orders;

  std::vector<boost::weak_ptr<const PnlRecord>> m_pnl;

  Implementation() = default;
  Implementation(
      const QUuid &id,
      QDateTime startTime,
      boost::shared_ptr<const StrategyInstanceRecord> strategyInstance,
      const OperationStatus &status)
      : m_id(id),
        m_status(status),
        m_startTime(std::move(startTime)),
        m_strategyInstance(std::move(strategyInstance)) {}
  Implementation(Implementation &&) = default;
  Implementation(const Implementation &) = default;
  Implementation &operator=(Implementation &&) = delete;
  Implementation &operator=(const Implementation &) = delete;
  ~Implementation() = default;
};

OperationRecord::OperationRecord()
    : m_pimpl(boost::make_unique<Implementation>()) {}

OperationRecord::OperationRecord(
    const QUuid &id,
    QDateTime startTime,
    boost::shared_ptr<const StrategyInstanceRecord> strategyInstance,
    const OperationStatus &status)
    : m_pimpl(boost::make_unique<Implementation>(
          id, std::move(startTime), std::move(strategyInstance), status)) {}
OperationRecord::OperationRecord(OperationRecord &&) noexcept = default;
OperationRecord::OperationRecord(const OperationRecord &rhs)
    : m_pimpl(boost::make_unique<Implementation>(*rhs.m_pimpl)) {}
OperationRecord &OperationRecord::operator=(OperationRecord &&) noexcept =
    default;
OperationRecord &OperationRecord::operator=(const OperationRecord &rhs) {
  OperationRecord(rhs).Swap(*this);
  return *this;
}
OperationRecord::~OperationRecord() = default;
void OperationRecord::Swap(OperationRecord &rhs) noexcept {
  m_pimpl.swap(rhs.m_pimpl);
}

const QUuid &OperationRecord::GetId() const { return m_pimpl->m_id; }
void OperationRecord::SetIdValue(const QUuid &id) { m_pimpl->m_id = id; }

const OperationStatus &OperationRecord::GetStatus() const {
  return m_pimpl->m_status;
}
void OperationRecord::SetStatus(const OperationStatus &status) {
  m_pimpl->m_status = status;
}

const QDateTime &OperationRecord::GetStartTime() const {
  return m_pimpl->m_startTime;
}
void OperationRecord::SetStartTimeValue(QDateTime time) {
  m_pimpl->m_startTime = std::move(time);
}

const boost::optional<QDateTime> &OperationRecord::GetEndTime() const {
  return m_pimpl->m_endTime;
}
void OperationRecord::SetEndTime(boost::optional<QDateTime> time) {
  m_pimpl->m_endTime = std::move(time);
}

const boost::shared_ptr<const StrategyInstanceRecord>
    &OperationRecord::GetStrategyInstance() const {
  return m_pimpl->m_strategyInstance;
}
void OperationRecord::SetStrategyInstanceValue(
    boost::shared_ptr<const StrategyInstanceRecord> strategyInstance) {
  m_pimpl->m_strategyInstance = std::move(strategyInstance);
}

const std::vector<boost::weak_ptr<const OrderRecord>>
    &OperationRecord::GetOrders() const {
  return m_pimpl->m_orders;
}
void OperationRecord::AddOrder(boost::weak_ptr<const OrderRecord> order) {
  m_pimpl->m_orders.emplace_back(std::move(order));
}
void OperationRecord::SetOrdersValue(
    std::vector<boost::weak_ptr<const OrderRecord>> list) {
  m_pimpl->m_orders = std::move(list);
}

const std::vector<boost::weak_ptr<const PnlRecord>> &OperationRecord::GetPnl()
    const {
  return m_pimpl->m_pnl;
}
std::vector<boost::weak_ptr<const PnlRecord>> &OperationRecord::GetPnl() {
  return m_pimpl->m_pnl;
}
void OperationRecord::SetPnlValue(
    std::vector<boost::weak_ptr<const PnlRecord>> pnl) {
  m_pimpl->m_pnl = std::move(pnl);
}
