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
using namespace Lib;
using namespace FrontEnd;
namespace front = FrontEnd;

namespace {

enum Column {
  COLUMN_SYMBOL,
  COLUMN_SOURCE,
  COLUMN_BID_PRICE,
  COLUMN_BID_QTY,
  COLUMN_ASK_PRICE,
  COLUMN_ASK_QTY,
  COLUMN_SPREAD,
  COLUMN_LAST_TIME,
  COLUMN_CURRENCY,
  COLUMN_TYPE_FULL,
  COLUMN_SYMBOL_FULL,
  numberOfColumns
};

QString ConvertToFullSecyrityTypeName(const SecurityType &source) {
  static_assert(numberOfSecurityTypes == 8, "List changed.");
  switch (source) {
    default:
      AssertEq(SECURITY_TYPE_STOCK, source);
      return QObject::tr("Unknown");
    case SECURITY_TYPE_STOCK:
      return QObject::tr("Stock");
    case SECURITY_TYPE_FUTURES:
      return QObject::tr("Future Contract");
    case SECURITY_TYPE_FUTURES_OPTIONS:
      return QObject::tr("Future Option Contract");
    case SECURITY_TYPE_FOR:
      return QObject::tr("Foreign Exchange Contract");
    case SECURITY_TYPE_FOR_FUTURES_OPTIONS:
      return QObject::tr(
          "Future Option Contract for Foreign Exchange Contract");
    case SECURITY_TYPE_OPTIONS:
      return QObject::tr("Option Contract");
    case SECURITY_TYPE_INDEX:
      return QObject::tr("Index");
    case SECURITY_TYPE_CRYPTO:
      return QObject::tr("Cryptocurrency");
  }
}

}  // namespace

class SecurityListModel::Implementation {
 public:
  Engine &m_engine;
  std::vector<Security *> m_securities;
};

SecurityListModel::SecurityListModel(front::Engine &engine, QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>({engine})) {
  Verify(connect(&m_pimpl->m_engine, &Engine::StateChange, this,
                 &SecurityListModel::OnStateChanged, Qt::QueuedConnection));
  Verify(connect(&m_pimpl->m_engine.GetDropCopy(), &DropCopy::PriceUpdate, this,
                 &SecurityListModel::UpdatePrice, Qt::QueuedConnection));
}
SecurityListModel::~SecurityListModel() = default;

void SecurityListModel::OnStateChanged(const bool isStarted) {
  isStarted ? Load() : Clear();
}

void SecurityListModel::UpdatePrice(const Security *security) {
  auto index = GetSecurityIndex(*security);
  if (index < 0) {
    Load();
    index = GetSecurityIndex(*security);
    if (index < 0) {
      AssertLe(0, index);
      return;
    }
  }
  dataChanged(createIndex(index, COLUMN_BID_PRICE),
              createIndex(index, COLUMN_LAST_TIME), {Qt::DisplayRole});
}

void SecurityListModel::Load() {
  std::vector<Security *> securities;
  for (size_t i = 0;
       i < m_pimpl->m_engine.GetContext().GetNumberOfMarketDataSources(); ++i) {
    m_pimpl->m_engine.GetContext().GetMarketDataSource(i).ForEachSecurity(
        [&securities](Security &security) {
          securities.emplace_back(&security);
        });
  }
  {
    beginResetModel();
    m_pimpl->m_securities = std::move(securities);
    endResetModel();
  }
}

void SecurityListModel::Clear() {
  beginResetModel();
  m_pimpl->m_securities.clear();
  endResetModel();
}

Security &SecurityListModel::GetSecurity(const QModelIndex &index) {
  return *static_cast<Security *>(index.internalPointer());
}

const Security &SecurityListModel::GetSecurity(const QModelIndex &index) const {
  return const_cast<SecurityListModel *>(this)->GetSecurity(index);
}

int SecurityListModel::GetSecurityIndex(const Security &security) const {
  const auto &it = std::find(m_pimpl->m_securities.cbegin(),
                             m_pimpl->m_securities.cend(), &security);
  if (it == m_pimpl->m_securities.cend()) {
    return -1;
  }
  return static_cast<int>(std::distance(m_pimpl->m_securities.cbegin(), it));
}

QVariant SecurityListModel::headerData(const int section,
                                       const Qt::Orientation orientation,
                                       const int role) const {
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
    case COLUMN_SPREAD:
      return tr("Spread");
    case COLUMN_LAST_TIME:
      return tr("Last time");
    case COLUMN_CURRENCY:
      return tr("Currency");
    case COLUMN_TYPE_FULL:
      return tr("Type");
    case COLUMN_SYMBOL_FULL:
      return tr("Symbol code");
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
          return QString::fromStdString(security.GetSource().GetTitle());
        case COLUMN_BID_PRICE:
          return security.GetBidPriceValue().Get();
        case COLUMN_BID_QTY:
          return security.GetBidQtyValue().Get();
        case COLUMN_ASK_PRICE:
          return security.GetAskPriceValue().Get();
        case COLUMN_ASK_QTY:
          return security.GetAskQtyValue().Get();
        case COLUMN_SPREAD:
          return (security.GetAskPriceValue() - security.GetBidPriceValue())
              .Get();
        case COLUMN_LAST_TIME: {
          return ConvertToQDateTime(security.GetLastMarketDataTime()).time();
        }
        case COLUMN_CURRENCY:
          return QString::fromStdString(
              ConvertToIso(security.GetSymbol().GetCurrency()));
        case COLUMN_SYMBOL_FULL:
          return QString::fromStdString(security.GetSymbol().GetAsString());
        case COLUMN_TYPE_FULL:
          return ConvertToFullSecyrityTypeName(
              security.GetSymbol().GetSecurityType());
        default:
          break;
      }
      break;

    case Qt::StatusTipRole:
    case Qt::ToolTipRole:
      return QString("%1 from %2")
          .arg(QString::fromStdString(security.GetSymbol().GetAsString()),
               QString::fromStdString(security.GetSource().GetTitle()));

    case Qt::TextAlignmentRole:
      return Qt::AlignLeft + Qt::AlignVCenter;
    default:
      break;
  }

  return QVariant();
}

QModelIndex SecurityListModel::index(const int row,
                                     const int column,
                                     const QModelIndex &) const {
  if (column < 0 || column > numberOfColumns || row < 0 ||
      row >= m_pimpl->m_securities.size()) {
    return {};
  }
  return createIndex(row, column, m_pimpl->m_securities[row]);
}

QModelIndex SecurityListModel::parent(const QModelIndex &) const { return {}; }

int SecurityListModel::rowCount(const QModelIndex &) const {
  return static_cast<int>(m_pimpl->m_securities.size());
}

int SecurityListModel::columnCount(const QModelIndex &) const {
  return numberOfColumns;
}
