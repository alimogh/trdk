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
  COLUMN_SYMBOL,
  COLUMN_EXCHANGE,
  COLUMN_STATUS,
  COLUMN_SIDE,
  COLUMN_PRICE,
  COLUMN_QTY,
  COLUMN_FILLED_QTY,
  COLUMN_REMAINING_QTY,
  COLUMN_TIME,
  COLUMN_LAST_TIME,
  COLUMN_TIF,
  COLUMN_TRADING_SYSTEM_ID,
  COLUMN_ID,
  numberOfColumns
};

struct Order {
  OrderId id;
  QVariant idStr;
  QVariant time;
  const Security *security;
  QVariant symbol;
  QVariant currency;
  const TradingSystem *tradingSystem;
  QVariant exchangeName;
  QVariant side;
  Qty qty;
  QVariant qtyStr;
  QVariant price;
  QVariant tif;
  mutable QVariant status;
  mutable QVariant filledQty;
  mutable QVariant remainingQty;
  mutable QVariant lastTime;
  mutable QVariant tradingSystemId;
};

struct ById {};
struct BySequnce {};

typedef boost::multi_index_container<
    Order,
    mi::indexed_by<
        mi::hashed_unique<
            mi::tag<ById>,
            mi::composite_key<
                Order,
                mi::member<Order, const TradingSystem *, &Order::tradingSystem>,
                mi::member<Order, OrderId, &Order::id>>>,
        mi::random_access<mi::tag<BySequnce>>>>
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
  const auto timeStr = QString::fromStdString(pt::to_simple_string(time));
  const auto qtyStr = ConvertQtyToText(qty, security->GetPricePrecision());
  const Order order{id,
                    QString::number(id),
                    timeStr,
                    security,
                    QString::fromStdString(security->GetSymbol().GetSymbol()),
                    QString::fromStdString(ConvertToIso(currency)),
                    tradingSystem,
                    QString::fromStdString(tradingSystem->GetInstanceName()),
                    QString(ConvertToPch(side)).toUpper(),
                    qty,
                    qtyStr,
                    ConvertPriceToText(price, security->GetPricePrecision()),
                    QString(ConvertToPch(tif)).toUpper(),
                    QString(ConvertToPch(ORDER_STATUS_SUBMITTED)),
                    ConvertPriceToText(0, security->GetPricePrecision()),
                    qtyStr,
                    timeStr};
  {
    const auto index = static_cast<int>(m_pimpl->m_orders.size());
    beginInsertRows(QModelIndex(), index, index);
    Verify(m_pimpl->m_orders.emplace(std::move(order)).second);
    endInsertRows();
  }
}

void OrderListModel::OnOrderUpdated(const trdk::OrderId &id,
                                    const std::string &tradingSystemId,
                                    const TradingSystem *tradingSystem,
                                    const pt::ptime &time,
                                    const OrderStatus &status,
                                    const Qty &remainingQty) {
  auto &orders = m_pimpl->m_orders.get<ById>();
  auto it = orders.find(boost::make_tuple(tradingSystem, id));
  Assert(it != orders.cend());
  if (it == orders.cend()) {
    return;
  }
  it->status = QString(ConvertToPch(status));
  if (!tradingSystemId.empty()) {
    it->tradingSystemId = QString::fromStdString(tradingSystemId);
  }
  it->lastTime = QString::fromStdString(pt::to_simple_string(time));
  it->filledQty = ConvertQtyToText(it->qty - remainingQty,
                                   it->security->GetPricePrecision());
  it->remainingQty =
      ConvertQtyToText(remainingQty, it->security->GetPricePrecision());
  {
    const auto &sequnce = m_pimpl->m_orders.get<BySequnce>();
    const auto index = static_cast<int>(std::distance(
        sequnce.cbegin(), m_pimpl->m_orders.project<BySequnce>(it)));
    dataChanged(createIndex(index, 0), createIndex(index, numberOfColumns - 1),
                {Qt::DisplayRole});
  }
}

void OrderListModel::OnOrder(const OrderId &id,
                             const std::string &tradingSystemId,
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
  auto it = orders.find(boost::make_tuple(tradingSystem, id));
  if (it == orders.cend()) {
    const Order order{id,
                      QString::number(id),
                      QString::fromStdString(pt::to_simple_string(openTime)),
                      nullptr,
                      QString::fromStdString(symbol),
                      QVariant(),
                      tradingSystem,
                      QString::fromStdString(tradingSystem->GetInstanceName()),
                      QString(ConvertToPch(side)).toUpper(),
                      qty,
                      ConvertQtyToText(qty, precision),
                      ConvertPriceToText(price, precision),
                      QString(ConvertToPch(tif)).toUpper(),
                      QString(ConvertToPch(status)),
                      ConvertPriceToText(qty - remainingQty, precision),
                      ConvertPriceToText(remainingQty, precision),
                      QString::fromStdString(pt::to_simple_string(updateTime)),
                      QString::fromStdString(tradingSystemId)};
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
    it->status = QString(ConvertToPch(status));
    it->filledQty = ConvertPriceToText(qty - remainingQty, precision);
    it->remainingQty = ConvertPriceToText(remainingQty, precision);
    it->lastTime = QString::fromStdString(pt::to_simple_string(updateTime));
    it->tradingSystemId = QString::fromStdString(tradingSystemId);
    {
      const auto &sequnce = m_pimpl->m_orders.get<BySequnce>();
      const auto index = static_cast<int>(std::distance(
          sequnce.cbegin(), m_pimpl->m_orders.project<BySequnce>(it)));
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
  static_assert(numberOfColumns == 13, "List changed.");
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
    case COLUMN_TRADING_SYSTEM_ID:
      return tr("ID");
    case COLUMN_ID:
      return tr("ID Int.");
  }
}

QVariant OrderListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  const auto &recordIndex = static_cast<size_t>(index.row());
  AssertGt(m_pimpl->m_orders.size(), recordIndex);
  const auto &order = m_pimpl->m_orders.get<BySequnce>()[recordIndex];

  switch (role) {
    case Qt::DisplayRole:
      static_assert(numberOfColumns == 13, "List changed.");
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
        case COLUMN_TRADING_SYSTEM_ID:
          return order.tradingSystemId;
        case COLUMN_ID:
          return order.idStr;
      }
    case Qt::TextAlignmentRole:
      return Qt::AlignLeft + Qt::AlignVCenter;
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
