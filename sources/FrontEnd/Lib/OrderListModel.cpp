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
#include "Core/Security.hpp"
#include "DropCopy.hpp"
#include "Engine.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::Lib;

namespace pt = boost::posix_time;
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

struct Order {
  QString id;
  QDateTime time;
  const Security *security;
  QString symbol;
  QString currency;
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
};

struct ById {};
struct BySequence {};

typedef boost::multi_index_container<
    Order,
    mi::indexed_by<
        mi::hashed_unique<
            mi::tag<ById>,
            mi::composite_key<
                Order,
                mi::member<Order, const TradingSystem *, &Order::tradingSystem>,
                mi::member<Order, QString, &Order::id>>>,
        mi::random_access<mi::tag<BySequence>>>>
    OrderList;
}

class OrderListModel::Implementation : private boost::noncopyable {
 public:
  OrderList m_orders;
};

OrderListModel::OrderListModel(Engine &engine, QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>()) {
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OrderSubmitted, this,
                 &OrderListModel::OnOrderSubmitted, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::OrderUpdated, this,
                 &OrderListModel::OnOrderUpdated, Qt::QueuedConnection));
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::Order, this,
                 &OrderListModel::OnOrder, Qt::QueuedConnection));
}

OrderListModel::~OrderListModel() = default;

void OrderListModel::OnOrderSubmitted(const OrderId &id,
                                      const pt::ptime &time,
                                      const Security *security,
                                      const Currency &currency,
                                      const TradingSystem *tradingSystem,
                                      const OrderSide &side,
                                      const Qty &qty,
                                      const boost::optional<Price> &price,
                                      const TimeInForce &tif) {
  const auto qtime = ConvertToQDateTime(time);
  const auto qtyStr = ConvertQtyToText(qty, security->GetPricePrecision());
  const Order order{QString::fromStdString(id.GetValue()),
                    qtime,
                    security,
                    QString::fromStdString(security->GetSymbol().GetSymbol()),
                    QString::fromStdString(ConvertToIso(currency)),
                    tradingSystem,
                    QString::fromStdString(tradingSystem->GetInstanceName()),
                    QString(ConvertToPch(side)).toLower(),
                    qty,
                    qtyStr,
                    ConvertPriceToText(price, security->GetPricePrecision()),
                    QString(ConvertToPch(tif)).toUpper(),
                    ConvertToPch(ORDER_STATUS_SUBMITTED),
                    ConvertPriceToText(0, security->GetPricePrecision()),
                    qtyStr,
                    qtime};
  {
    const auto index = static_cast<int>(m_pimpl->m_orders.size());
    beginInsertRows(QModelIndex(), index, index);
    Verify(m_pimpl->m_orders.emplace(std::move(order)).second);
    endInsertRows();
  }
}

void OrderListModel::OnOrderUpdated(const trdk::OrderId &id,
                                    const TradingSystem *tradingSystem,
                                    const pt::ptime &time,
                                    const OrderStatus &status,
                                    const Qty &remainingQty) {
  auto &orders = m_pimpl->m_orders.get<ById>();
  auto it = orders.find(
      boost::make_tuple(tradingSystem, QString::fromStdString(id.GetValue())));
  Assert(it != orders.cend());
  if (it == orders.cend()) {
    return;
  }
  it->status = ConvertToPch(status);
  it->lastTime = ConvertToQDateTime(time);
  it->filledQty = ConvertQtyToText(it->qty - remainingQty,
                                   it->security->GetPricePrecision());
  it->remainingQty =
      ConvertQtyToText(remainingQty, it->security->GetPricePrecision());
  {
    const auto &sequence = m_pimpl->m_orders.get<BySequence>();
    const auto index = static_cast<int>(std::distance(
        sequence.cbegin(), m_pimpl->m_orders.project<BySequence>(it)));
    dataChanged(createIndex(index, 0), createIndex(index, numberOfColumns - 1),
                {Qt::DisplayRole});
  }
}

void OrderListModel::OnOrder(const OrderId &id,
                             const TradingSystem *tradingSystem,
                             const std::string &symbol,
                             const OrderStatus &status,
                             const Qty &qty,
                             const Qty &remainingQty,
                             const boost::optional<Price> &price,
                             const OrderSide &side,
                             const TimeInForce &tif,
                             const pt::ptime &openTime,
                             const pt::ptime &updateTime) {
  uint8_t precision = 8;

  auto &orders = m_pimpl->m_orders.get<ById>();
  auto it = orders.find(
      boost::make_tuple(tradingSystem, QString::fromStdString(id.GetValue())));
  if (it == orders.cend()) {
    const Order order{QString::fromStdString(id.GetValue()),
                      ConvertToQDateTime(openTime),
                      nullptr,
                      QString::fromStdString(symbol),
                      QString(),
                      tradingSystem,
                      QString::fromStdString(tradingSystem->GetInstanceName()),
                      QString(ConvertToPch(side)).toUpper(),
                      qty,
                      ConvertQtyToText(qty, precision),
                      ConvertPriceToText(price, precision),
                      QString(ConvertToPch(tif)).toUpper(),
                      ConvertToPch(status),
                      ConvertPriceToText(qty - remainingQty, precision),
                      ConvertPriceToText(remainingQty, precision),
                      ConvertToQDateTime(updateTime)};
    {
      const auto index = static_cast<int>(m_pimpl->m_orders.size());
      beginInsertRows(QModelIndex(), index, index);
      Verify(m_pimpl->m_orders.emplace(std::move(order)).second);
      endInsertRows();
    }
  } else {
    if (it->security) {
      precision = it->security->GetPricePrecision();
    }
    it->status = ConvertToPch(status);
    it->filledQty = ConvertPriceToText(qty - remainingQty, precision);
    it->remainingQty = ConvertPriceToText(remainingQty, precision);
    it->lastTime = ConvertToQDateTime(updateTime);
    {
      const auto &sequence = m_pimpl->m_orders.get<BySequence>();
      const auto index = static_cast<int>(std::distance(
          sequence.cbegin(), m_pimpl->m_orders.project<BySequence>(it)));
      dataChanged(createIndex(index, 0),
                  createIndex(index, numberOfColumns - 1), {Qt::DisplayRole});
    }
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
    default:
      return Base::headerData(section, orientation, role);
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
      return tr("Quantity");
    case COLUMN_FILLED_QTY:
      return tr("Filled Qty.");
    case COLUMN_REMAINING_QTY:
      return tr("Remaining Qty.");
    case COLUMN_TIME:
      return tr("Time");
    case COLUMN_LAST_TIME:
      return tr("Update");
    case COLUMN_TIF:
      return tr("Time in Force");
    case COLUMN_ID:
      return tr("ID");
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
          return order.id;
      }
    case Qt::TextAlignmentRole:
      return Qt::AlignLeft + Qt::AlignVCenter;
    case ITEM_DATA_ROLE_ITEM_ID:
      return order.id;
    case ITEM_DATA_ROLE_TRADING_SYSTEM_INDEX:
      return order.tradingSystem->GetIndex();
    case ITEM_DATA_ROLE_TRADING_MODE:
      return order.tradingSystem->GetMode();
  }

  return QVariant();
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
