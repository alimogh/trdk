/*******************************************************************************
 *   Created: 2018/01/21 15:29:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BalanceListModel.hpp"
#include "Core/Balances.hpp"
#include "Core/Context.hpp"
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
  COLUMN_EXCHANGE,
  COLUMN_SYMBOL,
  COLUMN_AVAILABLE,
  COLUMN_LOCKED,
  COLUMN_UPDATE_TIME,
  numberOfColumns
};

struct Balance {
  QString symbol;
  double available;
  double locked;
  QTime time;

  explicit Balance(const std::string &symbol,
                   const Volume &available,
                   const Volume &locked)
      : symbol(QString::fromStdString(symbol)),
        available(available),
        locked(locked),
        time(QDateTime::currentDateTime().time()) {}

  void Update(const Volume &newAvailable, const Volume &newLocked) {
    available = newAvailable;
    locked = newLocked;
    time = QDateTime::currentDateTime().time();
  }
};

class Item : private boost::noncopyable {
 public:
  Item() : m_parent(nullptr), m_row(-1) {}
  virtual ~Item() = default;

 public:
  void AppendChild(std::unique_ptr<Item> &&child) {
    Assert(!child->m_parent);
    AssertEq(-1, child->m_row);
    boost::shared_ptr<Item> ptr(child.release());
    m_childItems.emplace_back(ptr);
    ptr->m_parent = this;
    ptr->m_row = static_cast<int>(m_childItems.size()) - 1;
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

class TradingSystemItem : public Item {
 public:
  explicit TradingSystemItem(const TradingSystem &tradingSystem)
      : m_data(QString::fromStdString(tradingSystem.GetInstanceName())) {}
  virtual ~TradingSystemItem() override = default;

 public:
  virtual QVariant GetData(int column) const override {
    if (column != COLUMN_EXCHANGE) {
      return QVariant();
    }
    return m_data;
  }

 private:
  QVariant m_data;
};

class RootItem : public Item {
 public:
  explicit RootItem() {}
  virtual ~RootItem() override = default;

 public:
  virtual QVariant GetData(int) const override { return QVariant(); }
};

class DataItem : public Item {
 public:
  explicit DataItem(const boost::shared_ptr<Balance> &data) : m_data(data) {}
  virtual ~DataItem() override = default;

 public:
  virtual QVariant GetData(int column) const override {
    switch (column) {
      case COLUMN_SYMBOL:
        return m_data->symbol;
      case COLUMN_AVAILABLE:
        return m_data->available;
      case COLUMN_LOCKED:
        return m_data->locked;
      case COLUMN_UPDATE_TIME:
        return m_data->time;
    }
    return QVariant();
  }

 private:
  const boost::shared_ptr<const Balance> m_data;
};
}

class BalanceListModel::Implementation : private boost::noncopyable {
 public:
  RootItem m_root;
  boost::unordered_map<
      const TradingSystem *,
      std::pair<TradingSystemItem *,
                boost::unordered_map<std::string, boost::shared_ptr<Balance>>>>
      m_data;
};

BalanceListModel::BalanceListModel(Engine &engine, QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>()) {
  if (engine.IsStarted()) {
    const auto &size = engine.GetContext().GetNumberOfTradingSystems();
    for (size_t i = 0; i < size; ++i) {
      const auto &tradingSystem =
          engine.GetContext().GetTradingSystem(i, TRADING_MODE_LIVE);
      tradingSystem.GetBalances().ForEach([this, &tradingSystem](
          const std::string &symbol, const Volume &available,
          const Volume &locked) {
        OnUpdate(&tradingSystem, symbol, available, locked);
      });
    }
  }
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::BalanceUpdate, this,
                 &BalanceListModel::OnUpdate, Qt::QueuedConnection));
}

BalanceListModel::~BalanceListModel() = default;

QVariant BalanceListModel::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const {
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
    return Base::headerData(section, orientation, role);
  }
  static_assert(numberOfColumns == 5, "List changed.");
  switch (section) {
    default:
      return Base::headerData(section, orientation, role);
    case COLUMN_EXCHANGE:
      return tr("Exchange");
    case COLUMN_SYMBOL:
      return tr("Symbol");
    case COLUMN_AVAILABLE:
      return tr("Available");
    case COLUMN_LOCKED:
      return tr("Locked");
    case COLUMN_UPDATE_TIME:
      return tr("Time");
  }
}

QVariant BalanceListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || role != Qt::DisplayRole) {
    return QVariant();
  }
  return static_cast<Item *>(index.internalPointer())->GetData(index.column());
}

QModelIndex BalanceListModel::index(int row,
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

QModelIndex BalanceListModel::parent(const QModelIndex &index) const {
  if (!index.isValid()) {
    return QModelIndex();
  }
  auto &parent = *static_cast<Item *>(index.internalPointer())->GetParent();
  if (&parent == &m_pimpl->m_root) {
    return QModelIndex();
  }
  return createIndex(parent.GetRow(), 0, &parent);
}

int BalanceListModel::rowCount(const QModelIndex &parentIndex) const {
  if (parentIndex.column() > 0) {
    return 0;
  }
  auto &parentItem = !parentIndex.isValid()
                         ? m_pimpl->m_root
                         : *static_cast<Item *>(parentIndex.internalPointer());
  return parentItem.GetNumberOfChilds();
}

int BalanceListModel::columnCount(const QModelIndex &parent) const {
  if (!parent.isValid()) {
    return m_pimpl->m_root.GetNumberOfColumns();
  }
  return static_cast<Item *>(parent.internalPointer())->GetNumberOfColumns();
}

Qt::ItemFlags BalanceListModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) {
    return 0;
  }
  return QAbstractItemModel::flags(index);
}

void BalanceListModel::OnUpdate(const TradingSystem *tradingSystem,
                                const std::string &symbol,
                                const Volume &available,
                                const Volume &locked) {
  auto tradingSystemIt = m_pimpl->m_data.find(tradingSystem);
  if (tradingSystemIt == m_pimpl->m_data.cend()) {
    if (available == 0 && locked == 0) {
      return;
    }
    const auto index = m_pimpl->m_root.GetNumberOfChilds();
    beginInsertRows(QModelIndex(), index, index);
    auto tradingSystemItem =
        boost::make_unique<TradingSystemItem>(*tradingSystem);
    tradingSystemIt =
        m_pimpl->m_data
            .emplace(std::make_pair(tradingSystem,
                                    std::make_pair(&*tradingSystemItem, 0)))
            .first;
    m_pimpl->m_root.AppendChild(std::move(tradingSystemItem));
    AssertEq(index, m_pimpl->m_root.GetNumberOfChilds() - 1);
    endInsertRows();
  }

  {
    auto &symbolList = tradingSystemIt->second.second;
    const auto &symbolIt = symbolList.find(symbol);
    if (symbolIt == symbolList.cend()) {
      if (available == 0 && locked == 0) {
        return;
      }
      const auto index = tradingSystemIt->second.first->GetNumberOfChilds();
      beginInsertRows(createIndex(tradingSystemIt->second.first->GetRow(), 0,
                                  tradingSystemIt->second.first),
                      index, index);
      const auto &data = boost::make_shared<Balance>(symbol, available, locked);
      tradingSystemIt->second.first->AppendChild(
          boost::make_unique<DataItem>(data));
      symbolList.emplace(std::make_pair(symbol, data));
      AssertEq(index, tradingSystemIt->second.first->GetNumberOfChilds() - 1);
      endInsertRows();
      return;
    }

    { symbolIt->second->Update(available, locked); }
  }
}
