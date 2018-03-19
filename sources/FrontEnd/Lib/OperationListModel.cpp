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
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Lib::Detail;

namespace pt = boost::posix_time;
namespace lib = trdk::FrontEnd::Lib;

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

  RootItem m_root;
  boost::unordered_map<QUuid, boost::shared_ptr<OperationNodeItem>>
      m_operations;
  boost::unordered_map<quint64, boost::shared_ptr<OperationOrderItem>> m_orders;

  explicit Implementation(OperationListModel &self) : m_self(self) {}

  void AddOrder(const Orm::Order &order,
                const boost::shared_ptr<OperationOrderItem> &&item) {
    const auto &operationIt = m_operations.find(order.getOperation()->getId());
    if (operationIt == m_operations.cend()) {
      Assert(operationIt != m_operations.cend());
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
};

OperationListModel::OperationListModel(lib::Engine &engine, QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>(*this)) {
  for (const auto &operation : engine.GetOperations()) {
    UpdateOperation(*operation);
    for (const auto &order : operation->getOrders()) {
      UpdateOrder(*order);
    }
  }

  Verify(connect(&engine, &Lib::Engine::OperationUpdate, this,
                 &OperationListModel::UpdateOperation));
  Verify(connect(&engine, &Lib::Engine::OrderUpdate, this,
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
  const auto &it = m_pimpl->m_operations.find(operation.getId());
  if (it == m_pimpl->m_operations.cend()) {
    const auto &record =
        boost::make_shared<OperationNodeItem>(OperationRecord(operation));
    Verify(
        m_pimpl->m_operations.emplace(std::make_pair(operation.getId(), record))
            .second);
    const auto &index = m_pimpl->m_root.GetNumberOfChilds();
    beginInsertRows(QModelIndex(), index, index);
    m_pimpl->m_root.AppendChild(record);
    endInsertRows();
  } else {
    auto &record = *it->second;
    record.GetRecord().Update(operation);
    emit dataChanged(
        createIndex(record.GetRow(), 0, &record),
        createIndex(record.GetRow(), numberOfOperationColumns - 1, &record));
  }
}

void OperationListModel::UpdateOrder(const Orm::Order &order) {
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
