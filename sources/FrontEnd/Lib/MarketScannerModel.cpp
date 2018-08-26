//
//    Created: 2018/08/26 3:22 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "MarketScannerModel.hpp"
#include "Engine.hpp"
#include "MarketOpportunityItem.hpp"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;
namespace front = FrontEnd;
namespace ptr = boost::property_tree;
namespace ids = boost::uuids;

namespace {
enum Column { COLUMN_PROFIT, COLUMN_STRATEGY, COLUMN_SYMBOLS, numberOfColumns };

const MarketOpportunityItem &GetMarketOpportunityItem(
    const QModelIndex &index) {
  return *static_cast<const MarketOpportunityItem *>(index.internalPointer());
}

class ArbirageAdvisorMarketOpportunityItem : public MarketOpportunityItem {
 public:
  explicit ArbirageAdvisorMarketOpportunityItem(const std::string &symbol,
                                                front::Engine &engine)
      : MarketOpportunityItem(QString::fromStdString(symbol),
                              CreateStrategy(engine, symbol)) {}
  ArbirageAdvisorMarketOpportunityItem(
      ArbirageAdvisorMarketOpportunityItem &&) = delete;
  ArbirageAdvisorMarketOpportunityItem(
      const ArbirageAdvisorMarketOpportunityItem &) = delete;
  ArbirageAdvisorMarketOpportunityItem &operator=(
      ArbirageAdvisorMarketOpportunityItem &&) = delete;
  ArbirageAdvisorMarketOpportunityItem &operator=(
      const ArbirageAdvisorMarketOpportunityItem &) = delete;
  ~ArbirageAdvisorMarketOpportunityItem() = default;

 private:
  static const Strategy &CreateStrategy(front::Engine &engine,
                                        const std::string &symbol) {
    static ids::random_generator generateUuid;
    const auto &id = generateUuid();
    {
      ptr::ptree namedConfig;
      namedConfig.add_child("Direct Arbitrage", CreateConfig(id, symbol));
      engine.GetContext().Add(namedConfig);
    }
    return engine.GetContext().GetSrategy(id);
  }

  static ptr::ptree CreateConfig(const ids::uuid &id,
                                 const std::string &symbol) {
    ptr::ptree result;
    result.add("module", "ArbitrationAdvisor");
    result.add("id", id);
    result.add("isEnabled", true);
    result.add("tradingMode", "live");
    {
      ptr::ptree symbols;
      symbols.push_back({"", ptr::ptree().put("", symbol)});
      result.add_child("requirements.level1Updates.symbols", symbols);
    }
    result.add("config.symbol", symbol);
    result.add("config.minPriceDifferenceToHighlightPercentage", .0);
    result.add("config.isAutoTradingEnabled", false);
    result.add("config.minPriceDifferenceToTradePercentage", .0);
    result.add("config.maxQty", 0);
    result.add("config.isLowestSpreadEnabled", false);
    result.add("config.lowestSpreadPercentage", 0);
    result.add("config.isStopLossEnabled", false);
    result.add("config.stopLossDelaySec", 0);
    return result;
  }
};

}  // namespace

class MarketScannerModel::Implementation {
 public:
  MarketScannerModel &m_self;
  front::Engine &m_engine;
  QTimer m_timer;

  boost::unordered_set<std::string> m_itemIndex;
  std::vector<boost::shared_ptr<MarketOpportunityItem>> m_items;

  explicit Implementation(front::Engine &engine, MarketScannerModel &self)
      : m_self(self), m_engine(engine), m_timer(&m_self) {}

  boost::unordered_set<std::string> LoadQuoteSymbols() const {
    boost::unordered_set<std::string> result;
    for (const auto &symbol : m_engine.GetContext().GetSymbolListHint()) {
      const auto delimeter = symbol.find('_');
      if (delimeter == std::string::npos) {
        continue;
      }
      auto quoteSymbol = symbol.substr(delimeter + 1);
      boost::trim(quoteSymbol);
      if (quoteSymbol.empty()) {
        continue;
      }
      result.emplace(std::move(quoteSymbol));
    }
    return result;
  }

  bool Refresh() {
    boost::optional<boost::unordered_set<std::string>> quoteSymbols;

    boost::unordered_set<std::string> pairs;
    for (const auto &symbol :
         m_engine.GetContext().GetSettings().GetDefaultSymbols()) {
      if (symbol.find('_') != std::string::npos) {
        pairs.emplace(symbol);
      } else {
        if (!quoteSymbols) {
          quoteSymbols = LoadQuoteSymbols();
        }
        for (const auto &quoteSymbol : *quoteSymbols) {
          pairs.emplace((symbol + '_').append(quoteSymbol));
        }
      }
    }

    size_t count = 0;
    for (const auto &pair : pairs) {
      if (!m_itemIndex.emplace(pair).second) {
        continue;
      }
      {
        const auto index = static_cast<int>(m_items.size());
        m_self.beginInsertRows(QModelIndex(), index, index);
        m_items.emplace_back(
            boost::make_shared<ArbirageAdvisorMarketOpportunityItem>(pair,
                                                                     m_engine));
        m_self.endInsertRows();
        m_self.connect(&*m_items.back(), &MarketOpportunityItem::ProfitUpdated,
                       &m_self,
                       [this, index]() {
                         m_self.beginInsertRows(QModelIndex(), index, index);
                         m_self.endInsertRows();
                       },
                       Qt::QueuedConnection);
      }
      if (++count >= 3) {
        return false;
      }
    }

    return true;
  }
};

MarketScannerModel::MarketScannerModel(front::Engine &engine, QWidget *parent)
    : Base(parent),
      m_pimpl(boost::make_unique<Implementation>(engine, *this)) {}
MarketScannerModel::~MarketScannerModel() = default;

QVariant MarketScannerModel::headerData(const int section,
                                        const Qt::Orientation orientation,
                                        const int role) const {
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
    return Base::headerData(section, orientation, role);
  }
  static_assert(numberOfColumns == 3, "List changed.");
  switch (section) {
    default:
      return Base::headerData(section, orientation, role);
    case COLUMN_PROFIT:
      return tr("Profit, %");
    case COLUMN_STRATEGY:
      return tr("Strategy");
    case COLUMN_SYMBOLS:
      return tr("Symbols");
  }
}

QVariant MarketScannerModel::data(const QModelIndex &index,
                                  const int role) const {
  if (!index.isValid()) {
    return {};
  }

  const auto &item = GetMarketOpportunityItem(index);

  switch (role) {
    case Qt::DisplayRole:
      static_assert(numberOfColumns == 3, "List changed.");
      switch (index.column()) {
        case COLUMN_PROFIT:
          return item.GetProfit();
        case COLUMN_STRATEGY:
          return QString::fromStdString(item.GetStrategy().GetInstanceName());
        case COLUMN_SYMBOLS:
          return item.GetSymbols();
        default:
          break;
      }
      break;

    case Qt::StatusTipRole:
    case Qt::ToolTipRole:
      return QString("%1: %2").arg(item.GetStrategy().GetInstanceName().c_str(),
                                   item.GetSymbols());

    case Qt::TextAlignmentRole:
      return Qt::AlignLeft + Qt::AlignVCenter;
    default:
      break;
  }

  return {};
}

QModelIndex MarketScannerModel::index(const int row,
                                      const int column,
                                      const QModelIndex &) const {
  if (column < 0 || column > numberOfColumns || row < 0 ||
      row >= static_cast<int>(m_pimpl->m_items.size())) {
    return {};
  }
  return createIndex(row, column, m_pimpl->m_items[row].get());
}

QModelIndex MarketScannerModel::parent(const QModelIndex &) const { return {}; }

int MarketScannerModel::rowCount(const QModelIndex &) const {
  return static_cast<int>(m_pimpl->m_items.size());
}

int MarketScannerModel::columnCount(const QModelIndex &) const {
  return numberOfColumns;
}

void MarketScannerModel::Refresh() {
  if (!m_pimpl->Refresh()) {
    m_pimpl->m_timer.singleShot(15 * 1000, this, &MarketScannerModel::Refresh);
  }
}
