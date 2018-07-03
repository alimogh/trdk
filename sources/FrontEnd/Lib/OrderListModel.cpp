/*******************************************************************************
 *   Created: 2017/10/16 21:47:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OrderListModel.hpp"
#include "DropCopy.hpp"
#include "Engine.hpp"
#include "I18n.hpp"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;
namespace mi = boost::multi_index;

namespace {

enum Column {
  COLUMN_TIME,
  COLUMN_SYMBOL,
  COLUMN_EXCHANGE,
  COLUMN_STATUS,
  COLUMN_SIDE,
  COLUMN_PRICE,
  COLUMN_QTY,
  COLUMN_FILLED_QTY,
  COLUMN_REMAINING_QTY,
  COLUMN_LAST_TIME,
  COLUMN_TIF,
  COLUMN_ID,
  numberOfColumns
};

struct OrderItem {
  QString id;
  QDateTime time;
  QString symbol;
  const TradingSystem *tradingSystem;
  QString exchangeName;
  QString side;
  Qty qty;
  QString qtyStr;
  QString price;
  QString tif;
  mutable QString status;
  mutable QString filledQty;
  mutable QString remainingQty;
  mutable QDateTime lastTime;
  QString submitError;
};

struct ById {};
struct BySequence {};

typedef boost::multi_index_container<
    OrderItem,
    mi::indexed_by<
        mi::hashed_unique<
            mi::tag<ById>,
            mi::composite_key<OrderItem,
                              mi::member<OrderItem,
                                         const TradingSystem *,
                                         &OrderItem::tradingSystem>,
                              mi::member<OrderItem, QString, &OrderItem::id>>>,
        mi::random_access<mi::tag<BySequence>>>>
    OrderList;

}  // namespace

class OrderListModel::Implementation {
 public:
  OrderList m_orders;
};

OrderListModel::OrderListModel(FrontEnd::Engine &engine, QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>()) {
  Verify(connect(&engine.GetDropCopy(), &DropCopy::FreeOrderSubmited, this,
                 &OrderListModel::SubmitOrder, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &DropCopy::FreeOrderSubmitError, this,
                 &OrderListModel::SubmitOrderError, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &DropCopy::OrderUpdated, this,
                 &OrderListModel::UpdateOrder, Qt::QueuedConnection));
}

OrderListModel::~OrderListModel() = default;

void OrderListModel::SubmitOrder(QString id,
                                 const QDateTime &time,
                                 const Security *security,
                                 const boost::shared_ptr<const Currency> &,
                                 const TradingSystem *tradingSystem,
                                 const boost::shared_ptr<const OrderSide> &side,
                                 const Qty &qty,
                                 const boost::optional<Price> &price,
                                 const TimeInForce &tif) {
  const auto qtyStr = ConvertQtyToText(qty);
  OrderItem order{std::move(id),
                  time,
                  QString::fromStdString(security->GetSymbol().GetSymbol()),
                  tradingSystem,
                  QString::fromStdString(tradingSystem->GetTitle()),
                  ConvertToUiString(*side),
                  qty,
                  qtyStr,
                  ConvertPriceToText(price),
                  QString(ConvertToPch(tif)).toUpper(),
                  ConvertToUiString(OrderStatus::Opened),
                  ConvertPriceToText(0),
                  qtyStr,
                  time,
                  {}};
  {
    const auto index = static_cast<int>(m_pimpl->m_orders.size());
    beginInsertRows(QModelIndex(), index, index);
    Verify(m_pimpl->m_orders.emplace(std::move(order)).second);
    endInsertRows();
  }
}

void OrderListModel::SubmitOrderError(
    const QDateTime &time,
    const Security *security,
    const Currency &,
    const TradingSystem *tradingSystem,
    const boost::shared_ptr<const OrderSide> &side,
    Qty qty,
    const boost::optional<Price> &price,
    const TimeInForce &tif,
    const QString &error) {
  const auto qtyStr = ConvertQtyToText(qty);
  const OrderItem order{
      QUuid::createUuid().toString(),
      time,
      QString::fromStdString(security->GetSymbol().GetSymbol()),
      tradingSystem,
      QString::fromStdString(tradingSystem->GetTitle()),
      ConvertToUiString(*side),
      std::move(qty),
      qtyStr,
      ConvertPriceToText(price),
      ConvertToUiString(tif),
      ConvertToUiString(OrderStatus::Error),
      ConvertPriceToText(0),
      qtyStr,
      time,
      !error.isEmpty() ? tr("Failed to submit order: %1").arg(error)
                       : tr("Unknown submit error")};
  {
    const auto index = static_cast<int>(m_pimpl->m_orders.size());
    beginInsertRows(QModelIndex(), index, index);
    Verify(m_pimpl->m_orders.emplace(std::move(order)).second);
    endInsertRows();
  }
}

void OrderListModel::UpdateOrder(
    const QString &id,
    const TradingSystem *tradingSystem,
    QDateTime time,
    const boost::shared_ptr<const OrderStatus> &status,
    const Qty &remainingQty) {
  auto &orders = m_pimpl->m_orders.get<ById>();
  auto it = orders.find(boost::make_tuple(tradingSystem, boost::cref(id)));
  if (it == orders.cend()) {
    // Maybe this is operation order.
    return;
  }
  it->status = ConvertToUiString(*status);
  it->lastTime = std::move(time);
  it->filledQty = ConvertQtyToText(it->qty - remainingQty);
  it->remainingQty = ConvertQtyToText(remainingQty);
  {
    const auto &sequence = m_pimpl->m_orders.get<BySequence>();
    const auto index = static_cast<int>(std::distance(
        sequence.cbegin(), m_pimpl->m_orders.project<BySequence>(it)));
    emit dataChanged(createIndex(index, 0),
                     createIndex(index, numberOfColumns - 1),
                     {Qt::DisplayRole});
  }
}

QVariant OrderListModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const {
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
    return Base::headerData(section, orientation, role);
  }
  static_assert(numberOfColumns == 12, "List changed.");
  switch (section) {
    case COLUMN_TIME:
      return tr("Time");
    case COLUMN_SYMBOL:
      return tr("Symbol");
    case COLUMN_EXCHANGE:
      return tr("Exchange");
    case COLUMN_STATUS:
      return tr("Status");
    case COLUMN_SIDE:
      return tr("Side");
    case COLUMN_PRICE:
      return tr("Price");
    case COLUMN_QTY:
      return tr("Order qty.");
    case COLUMN_FILLED_QTY:
      return tr("Filled qty.");
    case COLUMN_REMAINING_QTY:
      return tr("Remaining qty.");
    case COLUMN_LAST_TIME:
      return tr("Update");
    case COLUMN_TIF:
      return tr("Time In Force");
    case COLUMN_ID:
      return tr("ID");
    default:
      return "";
  }
}

QVariant OrderListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  const auto &recordIndex = static_cast<size_t>(index.row());
  AssertGt(m_pimpl->m_orders.size(), recordIndex);
  const auto &order = m_pimpl->m_orders.get<BySequence>()[recordIndex];

  switch (role) {
    case Qt::DisplayRole:
      static_assert(numberOfColumns == 12, "List changed.");
      switch (index.column()) {
        case COLUMN_SYMBOL:
          return order.symbol;
        case COLUMN_EXCHANGE:
          return order.exchangeName;
        case COLUMN_STATUS:
          return order.status;
        case COLUMN_SIDE:
          return order.side;
        case COLUMN_PRICE:
          return order.price;
        case COLUMN_QTY:
          return order.qtyStr;
        case COLUMN_FILLED_QTY:
          return order.filledQty;
        case COLUMN_REMAINING_QTY:
          return order.remainingQty;
        case COLUMN_TIME:
          return order.time;
        case COLUMN_LAST_TIME:
          return order.lastTime;
        case COLUMN_TIF:
          return order.tif;
        case COLUMN_ID:
          if (!order.submitError.isEmpty()) {
            {};
          }
          return order.id;
        default:
          return {};
      }
    case Qt::ToolTipRole:
      if (order.submitError.isEmpty()) {
        return {};
      }
      return order.submitError;
    case Qt::TextAlignmentRole:
      return Qt::AlignLeft + Qt::AlignVCenter;
    case ITEM_DATA_ROLE_ITEM_ID:
      return order.id;
    case ITEM_DATA_ROLE_TRADING_SYSTEM_INDEX:
      return order.tradingSystem->GetIndex();
    case ITEM_DATA_ROLE_TRADING_MODE:
      return order.tradingSystem->GetMode();
    default:
      return {};
  }
}

QModelIndex OrderListModel::index(int row,
                                  int column,
                                  const QModelIndex &) const {
  if (column < 0 || column > numberOfColumns || row < 0 ||
      row >= m_pimpl->m_orders.size()) {
    return QModelIndex();
  }
  return createIndex(row, column);
}

QModelIndex OrderListModel::parent(const QModelIndex &) const {
  return QModelIndex();
}

int OrderListModel::rowCount(const QModelIndex &) const {
  return static_cast<int>(m_pimpl->m_orders.size());
}

int OrderListModel::columnCount(const QModelIndex &) const {
  return numberOfColumns;
}
