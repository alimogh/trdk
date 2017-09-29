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
#include "ShellSecurityListModel.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "ShellDropCopy.hpp"
#include "ShellEngine.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::Shell;

namespace sh = trdk::FrontEnd::Shell;

namespace {
QVariant CheckSecurityFieldValue(double value) {
  if (isnan(value)) {
    return "-";
  }
  return value;
}
}

SecurityListModel::SecurityListModel(sh::Engine &engine, QWidget *parent)
    : Base(parent), m_engine(engine) {
  connect(&m_engine, &Engine::StateChanged, this,
          &SecurityListModel::StateChanged, Qt::QueuedConnection);

  connect(&m_engine.GetDropCopy(), &DropCopy::BidPriceUpdate, this,
          &SecurityListModel::BidPriceUpdate, Qt::QueuedConnection);
  connect(&m_engine.GetDropCopy(), &DropCopy::AskPriceUpdate, this,
          &SecurityListModel::AskPriceUpdate, Qt::QueuedConnection);
}

void SecurityListModel::StateChanged(bool isStarted) {
  isStarted ? Load() : Clear();
}

void SecurityListModel::BidPriceUpdate(const Security &security,
                                       const Price &) {
  UpdateValue(security, COLUMN_BID_PRICE);
}
void SecurityListModel::AskPriceUpdate(const Security &security,
                                       const Price &) {
  UpdateValue(security, COLUMN_ASK_PRICE);
}

void SecurityListModel::UpdateValue(const Security &security,
                                    const Column &column) {
  const auto &index = createIndex(GetSecurityIndex(security), column);
  dataChanged(index, index, {Qt::DisplayRole});
}

void SecurityListModel::Load() {
  std::vector<const Security *> securities;

  const auto &context = m_engine.GetContext();
  const auto &numberOfSource = context.GetNumberOfMarketDataSources();

  const auto &insertSecurity = [&securities](const Security &security) -> bool {
    securities.emplace_back(&security);
    return true;
  };
  for (size_t i = 0; i < numberOfSource; ++i) {
    auto &source = context.GetMarketDataSource(i);
    source.ForEachSecurity(insertSecurity);
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

const Security &SecurityListModel::GetSecurity(const QModelIndex &index) const {
  Assert(index.isValid());
  AssertLe(0, index.row());
  AssertGt(m_securities.size(), index.row());
  if (!index.isValid()) {
    throw LogicError("Security index is not valid");
  }
  return *m_securities[index.row()];
}

int SecurityListModel::GetSecurityIndex(const Security &security) const {
  const auto &it =
      std::find(m_securities.cbegin(), m_securities.cend(), &security);
  if (it == m_securities.cend()) {
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
  static_assert(numberOfColumns == 8, "List changed.");
  switch (section) {
    default:
      return Base::headerData(section, orientation, role);
    case COLUMN_SYMBOL:
      return tr("Symbol");
    case COLUMN_SOURCE:
      return tr("Source");
    case COLUMN_BID_PRICE:
      return tr("Bid");
    case COLUMN_ASK_PRICE:
      return tr("Ask");
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
      static_assert(numberOfColumns == 8, "List changed.");
      switch (index.column()) {
        case COLUMN_SYMBOL:
          return QString::fromStdString(security.GetSymbol().GetSymbol());
        case COLUMN_SOURCE:
          return QString::fromStdString(security.GetSource().GetInstanceName());
        case COLUMN_BID_PRICE:
          return CheckSecurityFieldValue(security.GetBidPriceValue());
        case COLUMN_ASK_PRICE:
          return CheckSecurityFieldValue(security.GetAskPriceValue());
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
  static_assert(numberOfSecurityTypes == 7, "List changed.");
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
  }
}
