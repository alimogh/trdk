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
enum { COLUMN_PROFIT, COLUMN_STRATEGY, COLUMN_SYMBOLS, numberOfColumns };

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
  ~ArbirageAdvisorMarketOpportunityItem() override = default;

  QString GetSymbolsConfig() const override { return GetSymbolsTitle(); }

  QString GetTitle() const override {
    return GetSymbolsTitle() + " " + tr("Arbitrage") + " - " +
           QCoreApplication::applicationName();
  }

 private:
  static const Strategy &CreateStrategy(front::Engine &engine,
                                        const std::string &symbol) {
    static ids::random_generator generateUuid;
    const auto &id = generateUuid();
    {
      ptr::ptree namedConfig;
      namedConfig.add_child("Direct", CreateConfig(id, symbol));
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

class TriangularArbitrageMarketOpportunityItem : public MarketOpportunityItem {
 public:
  explicit TriangularArbitrageMarketOpportunityItem(std::string leg1Symbol,
                                                    const OrderSide &leg1Side,
                                                    std::string leg2Symbol,
                                                    const OrderSide &leg2Side,
                                                    std::string leg3Symbol,
                                                    const OrderSide &leg3Side,
                                                    front::Engine &engine)
      : MarketOpportunityItem(CreateSymbolsTitle(leg1Symbol,
                                                 leg1Side,
                                                 leg2Symbol,
                                                 leg2Side,
                                                 leg3Symbol,
                                                 leg3Side),
                              CreateStrategy(engine,
                                             leg1Symbol,
                                             leg1Side,
                                             leg2Symbol,
                                             leg2Side,
                                             leg3Symbol,
                                             leg3Side)),
        m_leg1Symbol(std::move(leg1Symbol)),
        m_leg1Side(leg1Side),
        m_leg2Symbol(std::move(leg2Symbol)),
        m_leg2Side(leg2Side),
        m_leg3Symbol(std::move(leg3Symbol)),
        m_leg3Side(leg3Side) {}
  TriangularArbitrageMarketOpportunityItem(
      TriangularArbitrageMarketOpportunityItem &&) = delete;
  TriangularArbitrageMarketOpportunityItem(
      const TriangularArbitrageMarketOpportunityItem &) = delete;
  TriangularArbitrageMarketOpportunityItem &operator=(
      TriangularArbitrageMarketOpportunityItem &&) = delete;
  TriangularArbitrageMarketOpportunityItem &operator=(
      const TriangularArbitrageMarketOpportunityItem &) = delete;
  ~TriangularArbitrageMarketOpportunityItem() override = default;

  QString GetTitle() const override {
    QStringList symbolList;
    symbolList << QString("%1%2").arg(m_leg1Side == ORDER_SIDE_BUY ? "+" : "-",
                                      m_leg1Symbol.c_str());
    symbolList << QString("%1%2").arg(m_leg2Side == ORDER_SIDE_BUY ? "+" : "-",
                                      m_leg2Symbol.c_str());
    symbolList << QString("%1%2").arg(m_leg3Side == ORDER_SIDE_BUY ? "+" : "-",
                                      m_leg3Symbol.c_str());
    return symbolList.join('*') + " " + tr("Triangular Arbitrage") + " - " +
           QCoreApplication::applicationName();
  }

  QString GetSymbolsConfig() const override {
    std::ostringstream configStream;
    ptr::json_parser::write_json(
        configStream, GetSymbolsConfig(m_leg1Symbol, m_leg1Side, m_leg2Symbol,
                                       m_leg2Side, m_leg3Symbol, m_leg3Side));
    return QString::fromStdString(configStream.str());
  }

 private:
  static QString CreateSymbolsTitle(const std::string &leg1Symbol,
                                    const OrderSide &leg1Side,
                                    const std::string &leg2Symbol,
                                    const OrderSide &leg2Side,
                                    const std::string &leg3Symbol,
                                    const OrderSide &leg3Side) {
    return QString("%1%2, %3%4, %5%6")
        .arg(leg1Side == ORDER_SIDE_BUY ? "+" : "-", leg1Symbol.c_str(),
             leg2Side == ORDER_SIDE_BUY ? "+" : "-", leg2Symbol.c_str(),
             leg3Side == ORDER_SIDE_BUY ? "+" : "-", leg3Symbol.c_str());
  }

  static const Strategy &CreateStrategy(front::Engine &engine,
                                        const std::string &leg1Symbol,
                                        const OrderSide &leg1Side,
                                        const std::string &leg2Symbol,
                                        const OrderSide &leg2Side,
                                        const std::string &leg3Symbol,
                                        const OrderSide &leg3Side) {
    static ids::random_generator generateUuid;
    const auto &id = generateUuid();
    {
      ptr::ptree namedConfig;
      namedConfig.add_child("Triangular",
                            CreateConfig(id, leg1Symbol, leg1Side, leg2Symbol,
                                         leg2Side, leg3Symbol, leg3Side));
      engine.GetContext().Add(namedConfig);
    }
    return engine.GetContext().GetSrategy(id);
  }

  static ptr::ptree CreateConfig(const ids::uuid &id,
                                 const std::string &leg1Symbol,
                                 const OrderSide &leg1Side,
                                 const std::string &leg2Symbol,
                                 const OrderSide &leg2Side,
                                 const std::string &leg3Symbol,
                                 const OrderSide &leg3Side) {
    ptr::ptree result;
    result.add("module", "TriangularArbitrage");
    result.add("id", id);
    result.add("isEnabled", true);
    result.add("tradingMode", "live");
    result.add("config.isTradingEnabled", false);
    result.add("config.minVolume", 0);
    result.add("config.maxVolume", 1000);
    result.add("config.minProfitRatio", 1.01);
    result.add_child("config.legs",
                     GetSymbolsConfig(leg1Symbol, leg1Side, leg2Symbol,
                                      leg2Side, leg3Symbol, leg3Side));
    {
      ptr::ptree symbols;
      symbols.push_back({"", ptr::ptree().put("", leg1Symbol)});
      symbols.push_back({"", ptr::ptree().put("", leg2Symbol)});
      symbols.push_back({"", ptr::ptree().put("", leg3Symbol)});
      result.add_child("requirements.level1Updates.symbols", symbols);
    }
    return result;
  }

  static ptr::ptree GetSymbolsConfig(const std::string &leg1Symbol,
                                     const OrderSide &leg1Side,
                                     const std::string &leg2Symbol,
                                     const OrderSide &leg2Side,
                                     const std::string &leg3Symbol,
                                     const OrderSide &leg3Side) {
    ptr::ptree result;
    result.push_back(
        {"", ptr::ptree().put(
                 "", (boost::format("%1%%2%") %
                      (leg1Side == ORDER_SIDE_BUY ? '+' : '-') % leg1Symbol)
                         .str())});
    result.push_back(
        {"", ptr::ptree().put(
                 "", (boost::format("%1%%2%") %
                      (leg2Side == ORDER_SIDE_BUY ? '+' : '-') % leg2Symbol)
                         .str())});
    result.push_back(
        {"", ptr::ptree().put(
                 "", (boost::format("%1%%2%") %
                      (leg3Side == ORDER_SIDE_BUY ? '+' : '-') % leg3Symbol)
                         .str())});
    return result;
  }

  const std::string m_leg1Symbol;
  const OrderSide m_leg1Side;
  const std::string m_leg2Symbol;
  const OrderSide m_leg2Side;
  const std::string m_leg3Symbol;
  const OrderSide m_leg3Side;
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

  bool Refresh() {
    boost::unordered_set<std::string> quoteSymbols;
    boost::unordered_map<std::string, size_t> supportedSymbols;
    m_engine.GetContext().ForEachMarketDataSource(
        [&quoteSymbols, &supportedSymbols](const MarketDataSource &source) {
          for (auto symbol : source.GetSymbolListHint()) {
            const auto delimeter = symbol.find('_');
            if (delimeter == std::string::npos) {
              continue;
            }
            auto quoteSymbol = symbol.substr(delimeter + 1);
            boost::trim(quoteSymbol);
            if (quoteSymbol.empty()) {
              continue;
            }
            try {
              ConvertCurrencyFromIso(quoteSymbol);
            } catch (const Exception &) {
              continue;
            }
            const auto &it = supportedSymbols.emplace(std::move(symbol), 1);
            if (!it.second) {
              ++it.first->second;
            }
            quoteSymbols.emplace(quoteSymbol);
          }
        });

    typedef boost::unordered_map<std::string,
                                 std::pair<std::string, std::string>>
        Pairs;
    Pairs pairs;
    boost::unordered_map<std::string, std::vector<Pairs::const_iterator>>
        baseSymbols;

    for (const auto &symbol :
         m_engine.GetContext().GetSettings().GetDefaultSymbols()) {
      {
        const auto &it = supportedSymbols.find(symbol);
        if (it == supportedSymbols.cend() || it->second < 2) {
          continue;
        }
      }
      const auto delimeter = symbol.find('_');
      if (delimeter != std::string::npos) {
        auto baseSymbol = symbol.substr(0, delimeter);
        boost::trim(baseSymbol);
        if (baseSymbol.empty()) {
          continue;
        }
        auto quoteSymbol = symbol.substr(delimeter + 1);
        boost::trim(quoteSymbol);
        if (quoteSymbol.empty()) {
          continue;
        }
        const auto &it = pairs.emplace(
            symbol,
            std::make_pair(std::move(baseSymbol), std::move(quoteSymbol)));
        if (!it.second) {
          continue;
        }
        baseSymbols[it.first->second.first].emplace_back(it.first);
      } else {
        for (const auto &quoteSymbol : quoteSymbols) {
          auto pair = std::make_pair(symbol, quoteSymbol);
          const auto &it = pairs.emplace((pair.first + '_').append(quoteSymbol),
                                         std::move(pair));
          if (!it.second) {
            continue;
          }
          baseSymbols[symbol].emplace_back(it.first);
        }
      }
    }

    boost::unordered_map<std::string, std::vector<std::string>> leg2Pairs;
    for (const auto &leg1Pair : pairs) {
      const auto &leg1QuoteSymbol = leg1Pair.second.second;
      const auto &leg2BaseSymbols = baseSymbols.find(leg1QuoteSymbol);
      if (leg2BaseSymbols == baseSymbols.cend()) {
        continue;
      }

      std::vector<std::string> leg2PairsBuffer;
      for (const auto &leg2Pair : leg2BaseSymbols->second) {
        auto leg3Pair = leg1QuoteSymbol;
        leg3Pair += '_';
        leg3Pair += leg2Pair->second.second;
        if (pairs.count(leg3Pair)) {
          leg2PairsBuffer.emplace_back(leg2Pair->first);
        }
      }

      if (leg2PairsBuffer.empty()) {
        continue;
      }

      const auto &symbol = leg1Pair.first;
      Verify(leg2Pairs.emplace(symbol, std::move(leg2PairsBuffer)).second);
    }

    for (const auto &pair : pairs) {
      if (!m_itemIndex.emplace(pair.first).second) {
        continue;
      }
      AddItem(boost::make_shared<ArbirageAdvisorMarketOpportunityItem>(
          pair.first, m_engine));
    }

    for (const auto &leg2PairSet : leg2Pairs) {
      const auto &leg1Pair = leg2PairSet.first;
      AssertEq(1, pairs.count(leg1Pair));
      const auto &leg1Symbols = pairs[leg1Pair];
      for (const auto &leg2Pair : leg2PairSet.second) {
        AssertEq(1, pairs.count(leg2Pair));
        const auto &leg2Symbols = pairs[leg2Pair];
        const auto leg3Pair = leg1Symbols.first + "_" + leg2Symbols.second;
        if (m_itemIndex
                .emplace((boost::format("+%1%+%2%-%3%") % leg1Pair % leg2Pair %
                          leg3Pair)
                             .str())
                .second) {
          AddItem(boost::make_shared<TriangularArbitrageMarketOpportunityItem>(
              leg1Pair, ORDER_SIDE_BUY, leg2Pair, ORDER_SIDE_BUY, leg3Pair,
              ORDER_SIDE_SELL, m_engine));
        }
        if (m_itemIndex
                .emplace((boost::format("-%1%-%2%+%3%") % leg1Pair % leg2Pair %
                          leg3Pair)
                             .str())
                .second) {
          AddItem(boost::make_shared<TriangularArbitrageMarketOpportunityItem>(
              leg1Pair, ORDER_SIDE_SELL, leg2Pair, ORDER_SIDE_SELL, leg3Pair,
              ORDER_SIDE_BUY, m_engine));
        }
      }
    }

    return true;
  }

  void AddItem(boost::shared_ptr<MarketOpportunityItem> item) {
    const auto index = static_cast<int>(m_items.size());
    m_self.beginInsertRows(QModelIndex(), index, index);
    m_items.emplace_back(std::move(item));
    m_self.endInsertRows();
    Verify(m_self.connect(&*m_items.back(),
                          &MarketOpportunityItem::ProfitUpdated, &m_self,
                          [this, index]() {
                            emit m_self.dataChanged(
                                m_self.createIndex(index, 0),
                                m_self.createIndex(index, numberOfColumns - 1),
                                {Qt::DisplayRole});
                          },
                          Qt::QueuedConnection));
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
          return item.GetSymbolsTitle();
        default:
          break;
      }
      break;

    case Qt::StatusTipRole:
    case Qt::ToolTipRole:
      return QString("%1: %2").arg(item.GetStrategy().GetInstanceName().c_str(),
                                   item.GetSymbolsTitle());

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
