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
namespace ids = boost::uuids;
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
  boost::unordered_map<ids::uuid, boost::shared_ptr<OperationNodeItem>>
      m_operations;
  boost::unordered_map<std::pair<const TradingSystem *, std ::string>,
                       boost::shared_ptr<OperationOrderItem>>
      m_orders;

  explicit Implementation(OperationListModel &self) : m_self(self) {}

  void AddOrder(const ids::uuid &operationId,
                const std::string &orderId,
                const TradingSystem &tradingSystem,
                const boost::shared_ptr<OperationOrderItem> &&order) {
    const auto &operationIt = m_operations.find(operationId);
    if (operationIt == m_operations.cend()) {
      Assert(operationIt != m_operations.cend());
      return;
    }
    auto &operation = *operationIt->second;
    if (!m_orders
             .emplace(
                 std::make_pair(std::make_pair(&tradingSystem, orderId), order))
             .second) {
      Assert(false);
      return;
    }
    if (operation.GetNumberOfChilds() == 0) {
      m_self.beginInsertRows(
          m_self.createIndex(operation.GetRow(), 0, &operation), 0, 1);
      operation.AppendChild(boost::make_shared<OperationOrderHeadItem>());
    } else {
      const auto &index = operation.GetNumberOfChilds();
      m_self.beginInsertRows(
          m_self.createIndex(operation.GetRow(), 0, &operation), index, index);
    }
    operation.AppendChild(order);
    m_self.endInsertRows();
  }
};

OperationListModel::OperationListModel(lib::Engine &engine, QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>(*this)) {
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OperationStart, this,
                 &OperationListModel::AddOperation, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OperationUpdate, this,
                 &OperationListModel::UpdateOperation, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OperationEnd, this,
                 &OperationListModel::CompleteOperation, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OperationOrderSubmitted,
                 this, &OperationListModel::AddOrder, Qt::QueuedConnection));
  Verify(connect(
      &engine.GetDropCopy(), &Lib::DropCopy::OperationOrderSubmitError, this,
      &OperationListModel::AddOrderSumbitError, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OrderUpdated, this,
                 &OperationListModel::UpdateOrder, Qt::QueuedConnection));
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

void OperationListModel::AddOperation(const ids::uuid &id,
                                      const pt::ptime &time,
                                      const Strategy *strategy) {
  const auto &operation = boost::make_shared<OperationNodeItem>(
      OperationRecord(id, time, *strategy));
  Verify(m_pimpl->m_operations.emplace(std::make_pair(id, operation)).second);
  const auto &index = m_pimpl->m_root.GetNumberOfChilds();
  beginInsertRows(QModelIndex(), index, index);
  m_pimpl->m_root.AppendChild(operation);
  endInsertRows();
}

void OperationListModel::UpdateOperation(const ids::uuid &id,
                                         const Pnl::Data &data) {
  const auto &operationIt = m_pimpl->m_operations.find(id);
  if (operationIt == m_pimpl->m_operations.cend()) {
    Assert(operationIt != m_pimpl->m_operations.cend());
    return;
  }
  auto &operation = *operationIt->second;
  operation.GetRecord().Update(data);
  emit dataChanged(createIndex(operation.GetRow(), 0, &operation),
                   createIndex(operation.GetRow(), numberOfOperationColumns - 1,
                               &operation));
}

void OperationListModel::CompleteOperation(
    const ids::uuid &id,
    const pt::ptime &endTime,
    const boost::shared_ptr<const Pnl> &pnl) {
  const auto &operationIt = m_pimpl->m_operations.find(id);
  if (operationIt == m_pimpl->m_operations.cend()) {
    Assert(operationIt != m_pimpl->m_operations.cend());
    return;
  }
  auto &operation = *operationIt->second;
  operation.GetRecord().Complete(endTime, *pnl);
  emit dataChanged(createIndex(operation.GetRow(), 0, &operation),
                   createIndex(operation.GetRow(), numberOfOperationColumns - 1,
                               &operation));
}

void OperationListModel::AddOrder(const ids::uuid &operationId,
                                  int64_t subOperationId,
                                  const OrderId &orderId,
                                  const pt::ptime &orderTime,
                                  const Security *security,
                                  const Currency &currency,
                                  const TradingSystem *tradingSystem,
                                  const OrderSide &side,
                                  const Qty &qty,
                                  const boost::optional<Price> &price,
                                  const TimeInForce &tif) {
  m_pimpl->AddOrder(
      operationId, orderId.GetValue(), *tradingSystem,
      boost::make_shared<OperationOrderItem>(OrderRecord(
          QString::fromStdString(boost::lexical_cast<std::string>(orderId)),
          operationId, subOperationId, orderTime, *security, currency,
          *tradingSystem, side, qty, price, ORDER_STATUS_SENT, tif,
          QString())));
}

void OperationListModel::AddOrderSumbitError(
    const ids::uuid &operationId,
    int64_t subOperationId,
    const pt::ptime &orderTime,
    const Security *security,
    const Currency &currency,
    const TradingSystem *tradingSystem,
    const OrderSide &side,
    const Qty &qty,
    const boost::optional<Price> &price,
    const TimeInForce &tif,
    const QString &error) {
  static boost::uuids::random_generator generateOrderId;
  m_pimpl->AddOrder(
      operationId, boost::lexical_cast<std::string>(generateOrderId()),
      *tradingSystem,
      boost::make_shared<OperationOrderItem>(OrderRecord(
          QString(), operationId, subOperationId, orderTime, *security,
          currency, *tradingSystem, side, qty, price, ORDER_STATUS_ERROR, tif,
          !error.isEmpty() ? tr("Failed to submit order: %1").arg(error)
                           : tr("Unknown submit error"))));
}

void OperationListModel::UpdateOrder(const OrderId &orderId,
                                     const TradingSystem *tradingSystem,
                                     const pt::ptime &time,
                                     const OrderStatus &status,
                                     const Qty &remainingQty) {
  const auto &it = m_pimpl->m_orders.find(
      std::make_pair(tradingSystem, boost::cref(orderId.GetValue())));
  if (it == m_pimpl->m_orders.cend()) {
    // Maybe this is standalone order.
    return;
  }
  auto &order = *it->second;
  order.GetRecord().Update(time, status, remainingQty);
  emit dataChanged(
      createIndex(order.GetRow(), 0, &order),
      createIndex(order.GetRow(), numberOfOperationColumns - 1, &order));
}
