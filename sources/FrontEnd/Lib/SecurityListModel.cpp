/*******************************************************************************
 *   Created: 2017/09/29 11:28:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "SecurityListModel.hpp"
#include "DropCopy.hpp"
#include "Engine.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd;

namespace pt = boost::posix_time;
namespace front = trdk::FrontEnd;

SecurityListModel::SecurityListModel(front::Engine &engine, QWidget *parent)
    : Base(parent), m_engine(engine) {
  Verify(connect(&m_engine, &Engine::StateChange, this,
                 &SecurityListModel::OnStateChanged, Qt::QueuedConnection));
  Verify(connect(&m_engine.GetDropCopy(), &DropCopy::PriceUpdate, this,
                 &SecurityListModel::UpdatePrice, Qt::QueuedConnection));
}

void SecurityListModel::OnStateChanged(bool isStarted) {
  isStarted ? Load() : Clear();
}

void SecurityListModel::UpdatePrice(const Security *security) {
  const auto &index = GetSecurityIndex(*security);
  if (index < 0) {
    return;
  }
  dataChanged(createIndex(index, COLUMN_BID_PRICE),
              createIndex(index, COLUMN_LAST_TIME), {Qt::DisplayRole});
}

void SecurityListModel::Load() {
  std::vector<Security *> securities;
  for (size_t i = 0; i < m_engine.GetContext().GetNumberOfMarketDataSources();
       ++i) {
    m_engine.GetContext().GetMarketDataSource(i).ForEachSecurity(
        [&securities](Security &security) {
          securities.emplace_back(&security);
        });
  }
  {
    beginResetModel();
    securities.swap(m_securities);
    endResetModel();
  }
}

void SecurityListModel::Clear() {
  beginResetModel();
  m_securities.clear();
  endResetModel();
}

Security &SecurityListModel::GetSecurity(const QModelIndex &index) {
  Assert(index.isValid());
  AssertLe(0, index.row());
  AssertGt(m_securities.size(), index.row());
  if (!index.isValid()) {
    throw LogicError("Security index is not valid");
  }
  return *m_securities[index.row()];
}

const Security &SecurityListModel::GetSecurity(const QModelIndex &index) const {
  return const_cast<SecurityListModel *>(this)->GetSecurity(index);
}

int SecurityListModel::GetSecurityIndex(const Security &security) const {
  const auto &it =
      std::find(m_securities.cbegin(), m_securities.cend(), &security);
  if (it == m_securities.cend()) {
    if (m_securities.empty()) {
      return -1;
    }
    throw LogicError("Unknown security");
  }
  return static_cast<int>(std::distance(m_securities.cbegin(), it));
}

QVariant SecurityListModel::headerData(int section,
                                       Qt::Orientation orientation,
                                       int role) const {
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
    return Base::headerData(section, orientation, role);
  }
  static_assert(numberOfColumns == 11, "List changed.");
  switch (section) {
    default:
      return Base::headerData(section, orientation, role);
    case COLUMN_SYMBOL:
      return tr("Symbol");
    case COLUMN_SOURCE:
      return tr("Source");
    case COLUMN_BID_PRICE:
      return tr("Bid price");
    case COLUMN_BID_QTY:
      return tr("Bid qty");
    case COLUMN_ASK_PRICE:
      return tr("Ask price");
    case COLUMN_ASK_QTY:
      return tr("Ask qty");
    case COLUMN_LAST_TIME:
      return tr("Last time");
    case COLUMN_TYPE:
      return tr("Type");
    case COLUMN_CURRENCY:
      return tr("Currency");
    case COLUMN_TYPE_FULL:
      return tr("Full type name");
    case COLUMN_SYMBOL_FULL:
      return tr("Full symbol code");
  }
}

QVariant SecurityListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  const auto &security = GetSecurity(index);

  switch (role) {
    case Qt::DisplayRole:
      static_assert(numberOfColumns == 11, "List changed.");
      switch (index.column()) {
        case COLUMN_SYMBOL:
          return QString::fromStdString(security.GetSymbol().GetSymbol());
        case COLUMN_SOURCE:
          return QString::fromStdString(security.GetSource().GetInstanceName());
        case COLUMN_BID_PRICE:
          return ConvertPriceToText(security.GetBidPriceValue());
        case COLUMN_BID_QTY:
          return ConvertPriceToText(security.GetBidQtyValue());
        case COLUMN_ASK_PRICE:
          return ConvertPriceToText(security.GetAskPriceValue());
        case COLUMN_ASK_QTY:
          return ConvertPriceToText(security.GetAskQtyValue());
        case COLUMN_LAST_TIME: {
          return ConvertTimeToText(
              security.GetLastMarketDataTime().time_of_day());
        }
        case COLUMN_TYPE:
          return QString::fromStdString(
              ConvertToString(security.GetSymbol().GetSecurityType()));
        case COLUMN_CURRENCY:
          return QString::fromStdString(
              ConvertToIso(security.GetSymbol().GetCurrency()));
        case COLUMN_SYMBOL_FULL:
          return QString::fromStdString(security.GetSymbol().GetAsString());
        case COLUMN_TYPE_FULL:
          return ConvertFullToQString(security.GetSymbol().GetSecurityType());
      }

    case Qt::StatusTipRole:
    case Qt::ToolTipRole:
      return QString("%1 from %2")
          .arg(QString::fromStdString(security.GetSymbol().GetAsString()),
               QString::fromStdString(security.GetSource().GetInstanceName()));

    case Qt::TextAlignmentRole:
      return Qt::AlignLeft + Qt::AlignVCenter;
  }

  return QVariant();
}

QModelIndex SecurityListModel::index(int row,
                                     int column,
                                     const QModelIndex &) const {
  if (column < 0 || column > numberOfColumns || row < 0 ||
      row >= m_securities.size()) {
    return QModelIndex();
  }
  return createIndex(row, column);
}

QModelIndex SecurityListModel::parent(const QModelIndex &) const {
  return QModelIndex();
}

int SecurityListModel::rowCount(const QModelIndex &) const {
  return static_cast<int>(m_securities.size());
}

int SecurityListModel::columnCount(const QModelIndex &) const {
  return numberOfColumns;
}

QString SecurityListModel::ConvertFullToQString(
    const trdk::Lib::SecurityType &source) const {
  static_assert(numberOfSecurityTypes == 8, "List changed.");
  switch (source) {
    default:
      AssertEq(SECURITY_TYPE_STOCK, source);
      return tr("Unknown");
    case SECURITY_TYPE_STOCK:
      return tr("Stock");
    case SECURITY_TYPE_FUTURES:
      return tr("Future Contract");
    case SECURITY_TYPE_FUTURES_OPTIONS:
      return tr("Future Option Contract");
    case SECURITY_TYPE_FOR:
      return tr("Foreign Exchange Contract");
    case SECURITY_TYPE_FOR_FUTURES_OPTIONS:
      return tr("Future Option Contract for Foreign Exchange Contract");
    case SECURITY_TYPE_OPTIONS:
      return tr("Option Contract");
    case SECURITY_TYPE_INDEX:
      return tr("Index");
    case SECURITY_TYPE_CRYPTO:
      return tr("Cryptocurrency");
  }
}
