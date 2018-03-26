/*******************************************************************************
 *   Created: 2018/01/24 09:14:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OperationListModel.hpp"
#include "DropCopy.hpp"
#include "Engine.hpp"
#include "OperationItem.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Detail;

namespace pt = boost::posix_time;
namespace front = trdk::FrontEnd;

namespace {

class RootItem : public OperationItem {
 public:
  explicit RootItem() {}
  virtual ~RootItem() override = default;

 public:
  virtual QVariant GetData(int) const override { return QVariant(); }
};
}  // namespace

class OperationListModel::Implementation : private boost::noncopyable {
 public:
  OperationListModel &m_self;
  front::Engine &m_engine;

  RootItem m_root;
  boost::unordered_map<QUuid, boost::shared_ptr<OperationNodeItem>>
      m_operations;
  boost::unordered_map<quint64, boost::shared_ptr<OperationOrderItem>> m_orders;

  bool m_isTradesIncluded;
  bool m_isErrorsIncluded;
  bool m_isCancelsIncluded;

  QDate m_dateFrom;
  QDate m_dateTo;

  explicit Implementation(OperationListModel &self, front::Engine &engine)
      : m_self(self),
        m_engine(engine),
        m_isTradesIncluded(true),
        m_isErrorsIncluded(true),
        m_isCancelsIncluded(true),
        m_dateFrom(QDate::currentDate()),
        m_dateTo(m_dateFrom) {}

  void AddOrder(const Orm::Order &order,
                const boost::shared_ptr<OperationOrderItem> &&item) {
    const auto &operationIt = m_operations.find(order.getOperation()->getId());
    if (operationIt == m_operations.cend()) {
      return;
    }
    auto &record = *operationIt->second;
    if (!m_orders.emplace(order.getId(), item).second) {
      Assert(false);
      return;
    }
    if (record.GetNumberOfChilds() == 0) {
      m_self.beginInsertRows(m_self.createIndex(record.GetRow(), 0, &record), 0,
                             1);
      record.AppendChild(boost::make_shared<OperationOrderHeadItem>());
    } else {
      const auto &index = record.GetNumberOfChilds();
      m_self.beginInsertRows(m_self.createIndex(record.GetRow(), 0, &record),
                             index, index);
    }
    record.AppendChild(item);
    m_self.endInsertRows();
  }

  void Reload() {
    {
      m_self.beginResetModel();
      m_root.RemoveAllChildren();
      m_operations.clear();
      m_orders.clear();
      m_self.endResetModel();
    }

    for (const auto &operation :
         m_engine.GetOperations(m_isTradesIncluded, m_isErrorsIncluded,
                                m_isCancelsIncluded, m_dateFrom, m_dateTo)) {
      m_self.UpdateOperation(*operation);
      for (const auto &order : operation->getOrders()) {
        m_self.UpdateOrder(*order);
      }
    }
  }
};

OperationListModel::OperationListModel(front::Engine &engine, QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>(*this, engine)) {
  m_pimpl->Reload();
  Verify(connect(&engine, &Engine::OperationUpdate, this,
                 &OperationListModel::UpdateOperation));
  Verify(connect(&engine, &Engine::OrderUpdate, this,
                 &OperationListModel::UpdateOrder));
}

OperationListModel::~OperationListModel() = default;

QVariant OperationListModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const {
  if (orientation != Qt::Horizontal) {
    return Base::headerData(section, orientation, role);
  }
  if (role == Qt::TextAlignmentRole) {
    return GetOperationFieldAligment(static_cast<OperationColumn>(section));
  } else if (role == Qt::DisplayRole) {
    static_assert(numberOfOperationColumns == 14, "List changed.");
    switch (section) {
      case OPERATION_COLUMN_OPERATION_NUMBER_OR_ORDER_LEG:
        return tr("#");
      case OPERATION_COLUMN_OPERATION_TIME_OR_ORDER_SIDE:
        return tr("Start time");
      case OPERATION_COLUMN_OPERATION_END_TIME_OR_ORDER_TIME:
        return tr("End time");
      case OPERATION_COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL:
        return tr("Status");
      case OPERATION_COLUMN_OPERATION_FINANCIAL_RESULT_OR_ORDER_EXCHANGE:
        return tr("Financial result");
      case OPERATION_COLUMN_OPERATION_COMMISSION_OR_ORDER_STATUS:
        return tr("Commission");
      case OPERATION_COLUMN_OPERATION_TOTAL_RESULT_OR_ORDER_PRICE:
        return tr("Total result");
      case OPERATION_COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_QTY:
        return tr("Strategy");
      case OPERATION_COLUMN_OPERATION_STRATEGY_INSTANCE_OR_ORDER_VOLUME:
        return tr("Strategy instance");
      case OPERATION_COLUMN_OPERATION_ID_OR_ORDER_ID:
        return tr("ID");
    }
    return "";
  }
  return Base::headerData(section, orientation, role);
}

QVariant OperationListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }
  auto &item = *static_cast<OperationItem *>(index.internalPointer());
  switch (role) {
    case Qt::DisplayRole:
      return item.GetData(index.column());
    case Qt::ToolTipRole:
      return item.GetToolTip();
    case Qt::TextAlignmentRole:
      if (dynamic_cast<const OperationOrderItem *>(&item) ||
          dynamic_cast<const OperationOrderHeadItem *>(&item)) {
        return GetOperationFieldAligment(
            static_cast<OperationColumn>(index.column()));
      }
  }
  return QVariant();
}

QModelIndex OperationListModel::index(int row,
                                      int column,
                                      const QModelIndex &parentIndex) const {
  if (!hasIndex(row, column, parentIndex)) {
    return QModelIndex();
  }
  auto &parentItem =
      !parentIndex.isValid()
          ? m_pimpl->m_root
          : *static_cast<OperationItem *>(parentIndex.internalPointer());
  OperationItem *const childItem = parentItem.GetChild(row);
  if (!childItem) {
    return QModelIndex();
  }
  return createIndex(row, column, childItem);
}

QModelIndex OperationListModel::parent(const QModelIndex &index) const {
  if (!index.isValid()) {
    return QModelIndex();
  }
  auto &parent =
      *static_cast<OperationItem *>(index.internalPointer())->GetParent();
  if (&parent == &m_pimpl->m_root) {
    return QModelIndex();
  }
  return createIndex(parent.GetRow(), 0, &parent);
}

int OperationListModel::rowCount(const QModelIndex &parentIndex) const {
  if (parentIndex.column() > 0) {
    return 0;
  }
  auto &parentItem =
      !parentIndex.isValid()
          ? m_pimpl->m_root
          : *static_cast<OperationItem *>(parentIndex.internalPointer());
  return parentItem.GetNumberOfChilds();
}

int OperationListModel::columnCount(const QModelIndex &) const {
  return numberOfOperationColumns;
}

Qt::ItemFlags OperationListModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) {
    return 0;
  }
  return QAbstractItemModel::flags(index);
}

void OperationListModel::UpdateOperation(const Orm::Operation &operation) {
  bool isIncluded =
      operation.getStartTime().date() >= m_pimpl->m_dateFrom &&
      ((operation.getStatus() == Orm::OperationStatus::ACTIVE
            ? operation.getStartTime().date()
            : operation.getEndTime().date()) <= m_pimpl->m_dateTo) &&
      (m_pimpl->m_isTradesIncluded ||
       (operation.getStatus() != Orm::OperationStatus::LOSS &&
        operation.getStatus() != Orm::OperationStatus::PROFIT)) &&
      (m_pimpl->m_isErrorsIncluded ||
       operation.getStatus() != Orm::OperationStatus::ERROR) &&
      (m_pimpl->m_isCancelsIncluded ||
       operation.getStatus() != Orm::OperationStatus::CANCELED);

  const auto &it = m_pimpl->m_operations.find(operation.getId());
  if (it == m_pimpl->m_operations.cend()) {
    if (!isIncluded) {
      return;
    }
    const auto &record =
        boost::make_shared<OperationNodeItem>(OperationRecord(operation));
    Verify(
        m_pimpl->m_operations.emplace(std::make_pair(operation.getId(), record))
            .second);
    const auto &index = m_pimpl->m_root.GetNumberOfChilds();
    beginInsertRows(QModelIndex(), index, index);
    m_pimpl->m_root.AppendChild(record);
    endInsertRows();
  } else if (!isIncluded) {
    {
      const auto &record = it->second;
      beginRemoveRows(QModelIndex(), record->GetRow(), record->GetRow());
      m_pimpl->m_root.RemoveChild(record);
    }
    m_pimpl->m_operations.erase(it);
    endRemoveRows();
  } else {
    auto &record = *it->second;
    record.GetRecord().Update(operation);
    emit dataChanged(
        createIndex(record.GetRow(), 0, &record),
        createIndex(record.GetRow(), numberOfOperationColumns - 1, &record));
  }
}

void OperationListModel::UpdateOrder(const Orm::Order &order) {
  if (!order.getOperation()) {
    // "free" order
    return;
  }

  const auto &it = m_pimpl->m_orders.find(order.getId());
  if (it == m_pimpl->m_orders.cend()) {
    auto additionalInfo = order.getAdditionalInfo();
    m_pimpl->AddOrder(
        order,
        boost::make_shared<OperationOrderItem>(OrderRecord(
            order,
            order.getStatus() == ORDER_STATUS_SENT
                ? std::move(additionalInfo)
                : !additionalInfo.isEmpty()
                      ? tr("Failed to submit order: %1").arg(additionalInfo)
                      : tr("Unknown submit error"))));
  } else {
    auto &record = *it->second;
    record.GetRecord().Update(order);
    emit dataChanged(
        createIndex(record.GetRow(), 0, &record),
        createIndex(record.GetRow(), numberOfOperationColumns - 1, &record));
  }
}

void OperationListModel::Filter(const QDate &from, const QDate &to) {
  m_pimpl->m_dateFrom = from;
  m_pimpl->m_dateTo = to;
  m_pimpl->Reload();
}
void OperationListModel::DisableTimeFilter() {
  m_pimpl->m_dateFrom = QDate::currentDate();
  m_pimpl->m_dateTo = m_pimpl->m_dateFrom.addYears(100);
  m_pimpl->Reload();
}

void OperationListModel::IncludeTrades(bool isEnabled) {
  m_pimpl->m_isTradesIncluded = isEnabled;
  m_pimpl->Reload();
}
void OperationListModel::IncludeErrors(bool isEnabled) {
  m_pimpl->m_isErrorsIncluded = isEnabled;
  m_pimpl->Reload();
}
void OperationListModel::IncludeCancels(bool isEnabled) {
  m_pimpl->m_isCancelsIncluded = isEnabled;
  m_pimpl->Reload();
}
