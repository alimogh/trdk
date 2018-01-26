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
#include "OrderModelUtils.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Lib::Detail;

namespace pt = boost::posix_time;
namespace ids = boost::uuids;
namespace lib = trdk::FrontEnd::Lib;

namespace {

enum Column {
  COLUMN_OPERATION_NUMBER,
  COLUMN_OPERATION_TIME_OR_ORDER_TIME,
  COLUMN_OPERATION_END_TIME_OR_ORDER_LEG,
  COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL,
  COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_EXCHANGE,
  COLUMN_OPERATION_STRATEGY_INSTANCE_OR_ORDER_STATUS,
  COLUMN_OPERATION_ID_OR_ORDER_SIDE,
  COLUMN_ORDER_PRICE,
  COLUMN_ORDER_QTY,
  COLUMN_ORDER_FILLED_QTY,
  COLUMN_ORDER_REMAINING_QTY,
  COLUMN_ORDER_TIF,
  COLUMN_ORDER_LAST_TIME,
  COLUMN_ORDER_ID,
  numberOfColumns
};

struct OperationRecord {
  const QString id;
  const QTime startTime;
  const QString strategyName;
  const QString strategyInstance;
  QString status;
  QTime endTime;

  explicit OperationRecord(const ids::uuid &id,
                           const pt::ptime &startTime,
                           const Strategy &strategy)
      : id(QString::fromStdString(boost::lexical_cast<std::string>(id))),
        startTime(ConvertToQDateTime(startTime).time()),
        strategyName(QString::fromStdString(strategy.GetImplementationName())),
        strategyInstance(QString::fromStdString(strategy.GetInstanceName())),
        status(QObject::tr("performing")) {}

  void Update(const Pnl::Data &) {}

  void Complete(const pt::ptime &newEndTime, const Pnl &) {
    endTime = ConvertToQDateTime(newEndTime).time();
    status = QObject::tr("completed");
  }
};

class Item : private boost::noncopyable {
 public:
  Item() : m_parent(nullptr), m_row(-1) {}
  virtual ~Item() = default;

 public:
  void AppendChild(const boost::shared_ptr<Item> &child) {
    Assert(!child->m_parent);
    AssertEq(-1, child->m_row);
    m_childItems.emplace_back(child);
    child->m_parent = this;
    child->m_row = static_cast<int>(m_childItems.size()) - 1;
  }

  int GetRow() const {
    AssertLe(0, m_row);
    return m_row;
  }

  int GetNumberOfColumns() const { return numberOfColumns; }
  int GetNumberOfChilds() const {
    return static_cast<int>(m_childItems.size());
  }

  Item *GetChild(int row) {
    AssertGt(m_childItems.size(), row);
    if (m_childItems.size() <= row) {
      return nullptr;
    }
    return &*m_childItems[row];
  }

  Item *GetParent() { return m_parent; }

  virtual QVariant GetData(int column) const = 0;

 private:
  Item *m_parent;
  int m_row;
  std::vector<boost::shared_ptr<Item>> m_childItems;
};

class OperationItem : public Item {
 public:
  explicit OperationItem(OperationRecord &&record)
      : m_record(std::move(record)) {}
  virtual ~OperationItem() override = default;

 public:
  OperationRecord &GetRecord() { return m_record; }
  const OperationRecord &GetRecord() const { return m_record; }

  virtual QVariant GetData(int column) const override {
    static_assert(numberOfColumns == 14, "List changed.");
    switch (column) {
      case COLUMN_OPERATION_NUMBER:
        return GetRow() + 1;
      case COLUMN_OPERATION_TIME_OR_ORDER_TIME:
        return GetRecord().startTime;
      case COLUMN_OPERATION_END_TIME_OR_ORDER_LEG:
        return GetRecord().endTime;
      case COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL:
        return GetRecord().status;
      case COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_EXCHANGE:
        return GetRecord().strategyName;
      case COLUMN_OPERATION_STRATEGY_INSTANCE_OR_ORDER_STATUS:
        return GetRecord().strategyInstance;
      case COLUMN_OPERATION_ID_OR_ORDER_SIDE:
        return GetRecord().id;
    }
    return QVariant();
  }

 private:
  OperationRecord m_record;
};

class OrderHeadItem : public Item {
 public:
  virtual ~OrderHeadItem() override = default;

 public:
  virtual QVariant GetData(int column) const override {
    static_assert(numberOfColumns == 14, "List changed.");
    switch (column) {
      case COLUMN_OPERATION_TIME_OR_ORDER_TIME:
        return QObject::tr("Time");
      case COLUMN_OPERATION_END_TIME_OR_ORDER_LEG:
        return QObject::tr("Leg");
      case COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL:
        return QObject::tr("Symbol");
      case COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_EXCHANGE:
        return QObject::tr("Exchange");
      case COLUMN_OPERATION_STRATEGY_INSTANCE_OR_ORDER_STATUS:
        return QObject::tr("Status");
      case COLUMN_OPERATION_ID_OR_ORDER_SIDE:
        return QObject::tr("Side");
      case COLUMN_ORDER_PRICE:
        return QObject::tr("Order price");
      case COLUMN_ORDER_QTY:
        return QObject::tr("Order qty.");
      case COLUMN_ORDER_FILLED_QTY:
        return QObject::tr("Filled qty.");
      case COLUMN_ORDER_REMAINING_QTY:
        return QObject::tr("Remaining qty.");
      case COLUMN_ORDER_TIF:
        return QObject::tr("Time In Force");
      case COLUMN_ORDER_LAST_TIME:
        return QObject::tr("Update");
      case COLUMN_ORDER_ID:
        return QObject::tr("ID");
    }
    return "";
  }
};

class OrderItem : public Item {
 public:
  explicit OrderItem(OrderRecord &&record) : m_record(std::move(record)) {}
  virtual ~OrderItem() override = default;

 public:
  OrderRecord &GetRecord() { return m_record; }
  const OrderRecord &GetRecord() const { return m_record; }

  virtual QVariant GetData(int column) const override {
    static_assert(numberOfColumns == 14, "List changed.");
    switch (column) {
      case COLUMN_OPERATION_TIME_OR_ORDER_TIME:
        return GetRecord().orderTime;
      case COLUMN_OPERATION_END_TIME_OR_ORDER_LEG:
        return *GetRecord().subOperationId;
      case COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL:
        return GetRecord().symbol;
      case COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_EXCHANGE:
        return GetRecord().exchangeName;
      case COLUMN_OPERATION_STRATEGY_INSTANCE_OR_ORDER_STATUS:
        return GetRecord().status;
      case COLUMN_OPERATION_ID_OR_ORDER_SIDE:
        return GetRecord().side;
      case COLUMN_ORDER_PRICE:
        return GetRecord().price;
      case COLUMN_ORDER_QTY:
        return GetRecord().qty;
      case COLUMN_ORDER_FILLED_QTY:
        return GetRecord().filledQty;
      case COLUMN_ORDER_REMAINING_QTY:
        return GetRecord().remainingQty;
      case COLUMN_ORDER_TIF:
        return GetRecord().tif;
      case COLUMN_ORDER_LAST_TIME:
        return GetRecord().lastTime;
      case COLUMN_ORDER_ID:
        return GetRecord().id;
    }
    return QVariant();
  }

 private:
  OrderRecord m_record;
};

class RootItem : public Item {
 public:
  explicit RootItem() {}
  virtual ~RootItem() override = default;

 public:
  virtual QVariant GetData(int) const override { return QVariant(); }
};
}  // namespace

class OperationListModel::Implementation : private boost::noncopyable {
 public:
  RootItem m_root;
  boost::unordered_map<ids::uuid, boost::shared_ptr<OperationItem>>
      m_operations;
  boost::unordered_map<std::pair<const TradingSystem *, OrderId>,
                       boost::shared_ptr<OrderItem>>
      m_orders;
};

OperationListModel::OperationListModel(lib::Engine &engine, QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>()) {
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OperationStart, this,
                 &OperationListModel::AddOperation, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OperationUpdate, this,
                 &OperationListModel::UpdateOperation, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OperationEnd, this,
                 &OperationListModel::CompleteOperation, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OperationOrderSubmitted,
                 this, &OperationListModel::AddOrder, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OrderUpdated, this,
                 &OperationListModel::UpdateOrder, Qt::QueuedConnection));
}

OperationListModel::~OperationListModel() = default;

QVariant OperationListModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const {
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
    return Base::headerData(section, orientation, role);
  }
  static_assert(numberOfColumns == 14, "List changed.");
  switch (section) {
    case COLUMN_OPERATION_NUMBER:
      return tr("#");
    case COLUMN_OPERATION_TIME_OR_ORDER_TIME:
      return tr("Start time");
    case COLUMN_OPERATION_END_TIME_OR_ORDER_LEG:
      return tr("End time");
    case COLUMN_OPERATION_STATUS_OR_ORDER_SYMBOL:
      return tr("Status");
    case COLUMN_OPERATION_STRATEGY_NAME_OR_ORDER_EXCHANGE:
      return tr("Strategy");
    case COLUMN_OPERATION_STRATEGY_INSTANCE_OR_ORDER_STATUS:
      return tr("Strategy instance");
    case COLUMN_OPERATION_ID_OR_ORDER_SIDE:
      return tr("ID");
  }
  return "";
}

QVariant OperationListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || role != Qt::DisplayRole) {
    return QVariant();
  }
  return static_cast<Item *>(index.internalPointer())->GetData(index.column());
}

QModelIndex OperationListModel::index(int row,
                                      int column,
                                      const QModelIndex &parentIndex) const {
  if (!hasIndex(row, column, parentIndex)) {
    return QModelIndex();
  }
  auto &parentItem = !parentIndex.isValid()
                         ? m_pimpl->m_root
                         : *static_cast<Item *>(parentIndex.internalPointer());
  Item *const childItem = parentItem.GetChild(row);
  if (!childItem) {
    return QModelIndex();
  }
  return createIndex(row, column, childItem);
}

QModelIndex OperationListModel::parent(const QModelIndex &index) const {
  if (!index.isValid()) {
    return QModelIndex();
  }
  auto &parent = *static_cast<Item *>(index.internalPointer())->GetParent();
  if (&parent == &m_pimpl->m_root) {
    return QModelIndex();
  }
  return createIndex(parent.GetRow(), 0, &parent);
}

int OperationListModel::rowCount(const QModelIndex &parentIndex) const {
  if (parentIndex.column() > 0) {
    return 0;
  }
  auto &parentItem = !parentIndex.isValid()
                         ? m_pimpl->m_root
                         : *static_cast<Item *>(parentIndex.internalPointer());
  return parentItem.GetNumberOfChilds();
}

int OperationListModel::columnCount(const QModelIndex &parent) const {
  if (!parent.isValid()) {
    return m_pimpl->m_root.GetNumberOfColumns();
  }
  return static_cast<Item *>(parent.internalPointer())->GetNumberOfColumns();
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
  const auto &operation =
      boost::make_shared<OperationItem>(OperationRecord(id, time, *strategy));
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
                   createIndex(operation.GetRow(),
                               operation.GetNumberOfColumns() - 1, &operation));
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
                   createIndex(operation.GetRow(),
                               operation.GetNumberOfColumns() - 1, &operation));
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
  const auto &operationIt = m_pimpl->m_operations.find(operationId);
  if (operationIt == m_pimpl->m_operations.cend()) {
    Assert(operationIt != m_pimpl->m_operations.cend());
    return;
  }
  auto &operation = *operationIt->second;
  const auto &order = boost::make_shared<OrderItem>(OrderRecord(
      OrderRecord(orderId, operationId, subOperationId, orderTime, *security,
                  currency, *tradingSystem, side, qty, price, tif)));
  if (!m_pimpl->m_orders
           .emplace(
               std::make_pair(std::make_pair(tradingSystem, orderId), order))
           .second) {
    Assert(false);
    return;
  }
  if (operation.GetNumberOfChilds() == 0) {
    beginInsertRows(createIndex(operation.GetRow(), 0, &operation), 0, 1);
    operation.AppendChild(boost::make_shared<OrderHeadItem>());
  } else {
    const auto &index = operation.GetNumberOfChilds();
    beginInsertRows(createIndex(operation.GetRow(), 0, &operation), index,
                    index);
  }
  operation.AppendChild(order);
  endInsertRows();
}

void OperationListModel::UpdateOrder(const OrderId &orderId,
                                     const TradingSystem *tradingSystem,
                                     const pt::ptime &time,
                                     const OrderStatus &status,
                                     const Qty &remainingQty) {
  const auto &it = m_pimpl->m_orders.find(
      std::make_pair(tradingSystem, boost::cref(orderId)));
  if (it == m_pimpl->m_orders.cend()) {
    // Maybe this is standalone order.
    return;
  }
  auto &order = *it->second;
  order.GetRecord().Update(time, status, remainingQty);
  emit dataChanged(
      createIndex(order.GetRow(), 0, &order),
      createIndex(order.GetRow(), order.GetNumberOfColumns() - 1, &order));
}
