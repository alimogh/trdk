//
//    Created: 2018/08/26 2:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "MarketScannerView.hpp"
#include "MarketOpportunityItem.hpp"
#include "MarketOpportunityItemDelegate.hpp"

using namespace trdk::FrontEnd;

class MarketScannerView::Implementation {
 public:
  MarketScannerView &m_self;

  size_t m_numberOfResizes = 0;

  explicit Implementation(MarketScannerView &self) : m_self(self) {}

  void InitContextMenu() {
    m_self.setContextMenuPolicy(Qt::ActionsContextMenu);
    {
      auto &action = *new QAction(m_self.tr("Start Trading"), &m_self);
      Verify(m_self.connect(&action, &QAction::triggered, [this]() {
        for (auto &index : m_self.selectionModel()->selectedIndexes()) {
          RequestStrategy(index);
        }
      }));
      m_self.addAction(&action);
    }
  }

  void RequestStrategy(const QModelIndex &index) {
    const auto &item = ResolveModelIndexItem<MarketOpportunityItem>(index);
    emit m_self.StrategyRequested(
        item.GetTitle(),
        QString::fromStdString(item.GetStrategy().GetModuleName()),
        "CreateStrategyWidgetsForSymbols", item.GetSymbolsConfig());
  }
};

MarketScannerView::MarketScannerView(QWidget *parent)
    : Base(parent), m_pimpl(boost::make_unique<Implementation>(*this)) {
  setWindowTitle(tr("Market Scanner"));
  setSortingEnabled(true);
  setAlternatingRowColors(true);
  setSelectionBehavior(SelectRows);
  setSelectionMode(SingleSelection);
  verticalHeader()->setVisible(false);
  verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  verticalHeader()->setDefaultSectionSize(
      verticalHeader()->fontMetrics().height() + 4);
  horizontalHeader()->setStretchLastSection(true);
  setItemDelegate(new MarketOpportunityItemDelegate(this));
  Verify(connect(
      this, &MarketScannerView::doubleClicked,
      [this](const QModelIndex &index) { m_pimpl->RequestStrategy(index); }));
  m_pimpl->InitContextMenu();
}
MarketScannerView::~MarketScannerView() = default;

void MarketScannerView::rowsInserted(const QModelIndex &index,
                                     const int start,
                                     const int end) {
  Base::rowsInserted(index, start, end);
  if (m_pimpl->m_numberOfResizes < 3) {
    for (auto i = 0; i < horizontalHeader()->count(); ++i) {
      resizeColumnToContents(i);
    }
    ++m_pimpl->m_numberOfResizes;
  }
}
