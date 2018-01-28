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
#include "BalanceItem.hpp"
#include "DropCopy.hpp"
#include "Engine.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Lib::Detail;

namespace pt = boost::posix_time;
namespace lib = trdk::FrontEnd::Lib;

namespace {

class RootItem : public BalanceItem {
 public:
  explicit RootItem() {}
  virtual ~RootItem() override = default;

 public:
  virtual QVariant GetData(int) const override { return QVariant(); }
};

}  // namespace

class BalanceListModel::Implementation : private boost::noncopyable {
 public:
  RootItem m_root;
  boost::unordered_map<
      const TradingSystem *,
      std::pair<BalanceTradingSystemItem *,
                boost::unordered_map<std::string,
                                     boost::shared_ptr<BalanceDataItem>>>>
      m_data;
  boost::unordered_set<std::string> m_defaultSymbols;
};

BalanceListModel::BalanceListModel(lib::Engine &engine, QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>()) {
  if (engine.IsStarted()) {
    const auto &size = engine.GetContext().GetNumberOfTradingSystems();
    for (size_t i = 0; i < size; ++i) {
      const auto &tradingSystem =
          engine.GetContext().GetTradingSystem(i, TRADING_MODE_LIVE);
      tradingSystem.GetBalances().ForEach(
          [this, &tradingSystem](const std::string &symbol,
                                 const Volume &available,
                                 const Volume &locked) {
            OnUpdate(&tradingSystem, symbol, available, locked);
          });
    }
  }
  {
    const IniFile conf(engine.GetConfigFilePath());
    const IniSectionRef defaults(conf, "Defaults");
    for (const std::string &symbol :
         defaults.ReadList("symbol_list", ",", true)) {
      for (auto it = boost::make_split_iterator(
               symbol, boost::first_finder("_", boost::is_iequal()));
           !it.eof(); ++it) {
        auto subSymbol = boost::copy_range<std::string>(*it);
        if (!subSymbol.empty()) {
          m_pimpl->m_defaultSymbols.emplace(std::move(subSymbol));
        }
      }
    }
  }
  Verify(connect(&engine.GetDropCopy(), &Lib::DropCopy::BalanceUpdate, this,
                 &BalanceListModel::OnUpdate, Qt::QueuedConnection));
}

BalanceListModel::~BalanceListModel() = default;

QVariant BalanceListModel::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const {
  if (orientation != Qt::Horizontal) {
    return Base::headerData(section, orientation, role);
  }

  if (role == Qt::TextAlignmentRole) {
    return GetBalanceFieldAligment(static_cast<BalanceColumn>(section));
  } else if (role == Qt::DisplayRole) {
    static_assert(numberOfBalanceColumns == 6, "List changed.");
    switch (section) {
      case BALANCE_COLUMN_EXCHANGE_OR_SYMBOL:
        return tr("Symbol");
      case BALANCE_COLUMN_AVAILABLE:
        return tr("Available");
      case BALANCE_COLUMN_LOCKED:
        return tr("Locked");
      case BALANCE_COLUMN_TOTAL:
        return tr("Total");
      case BALANCE_COLUMN_UPDATE_TIME:
        return tr("Time");
      case BALANCE_COLUMN_USED:
        return tr("Used");
    }
  }

  return Base::headerData(section, orientation, role);
}

QVariant BalanceListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }
  switch (role) {
    case Qt::TextAlignmentRole:
      return GetBalanceFieldAligment(
          static_cast<BalanceColumn>(index.column()));
    case Qt::DisplayRole:
      return static_cast<BalanceItem *>(index.internalPointer())
          ->GetData(index.column());
  }
  return QVariant();
}

QModelIndex BalanceListModel::index(int row,
                                    int column,
                                    const QModelIndex &parentIndex) const {
  if (!hasIndex(row, column, parentIndex)) {
    return QModelIndex();
  }
  auto &parentItem =
      !parentIndex.isValid()
          ? m_pimpl->m_root
          : *static_cast<BalanceItem *>(parentIndex.internalPointer());
  BalanceItem *const childItem = parentItem.GetChild(row);
  if (!childItem) {
    return QModelIndex();
  }
  return createIndex(row, column, childItem);
}

QModelIndex BalanceListModel::parent(const QModelIndex &index) const {
  if (!index.isValid()) {
    return QModelIndex();
  }
  auto &parent =
      *static_cast<BalanceItem *>(index.internalPointer())->GetParent();
  if (&parent == &m_pimpl->m_root) {
    return QModelIndex();
  }
  return createIndex(parent.GetRow(), 0, &parent);
}

int BalanceListModel::rowCount(const QModelIndex &parentIndex) const {
  if (parentIndex.column() > 0) {
    return 0;
  }
  auto &parentItem =
      !parentIndex.isValid()
          ? m_pimpl->m_root
          : *static_cast<BalanceItem *>(parentIndex.internalPointer());
  return parentItem.GetNumberOfChilds();
}

int BalanceListModel::columnCount(const QModelIndex &) const {
  return numberOfBalanceColumns;
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
        boost::make_unique<BalanceTradingSystemItem>(*tradingSystem);
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
      const bool isUsed = m_pimpl->m_defaultSymbols.count(symbol) > 0;
      if (!isUsed && available == 0 && locked == 0) {
        return;
      }
      const auto index = tradingSystemIt->second.first->GetNumberOfChilds();
      const auto &data =
          boost::make_shared<BalanceRecord>(symbol, available, locked, isUsed);
      const auto &item = boost::make_shared<BalanceDataItem>(data);
      beginInsertRows(createIndex(tradingSystemIt->second.first->GetRow(), 0,
                                  tradingSystemIt->second.first),
                      index, index);
      tradingSystemIt->second.first->AppendChild(item);
      symbolList.emplace(std::make_pair(symbol, item));
      AssertEq(index, tradingSystemIt->second.first->GetNumberOfChilds() - 1);
      endInsertRows();
      return;
    }

    {
      auto &data = *symbolIt->second;
      data.GetRecord().Update(available, locked);
      emit dataChanged(
          createIndex(data.GetRow(), 0, &data),
          createIndex(data.GetRow(), numberOfBalanceColumns - 1, &data));
    }
  }
}
