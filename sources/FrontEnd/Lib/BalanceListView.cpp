/*******************************************************************************
 *   Created: 2018/01/21 16:12:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "BalanceListView.hpp"
#include "BalanceItem.hpp"
#include "BalanceItemDelegate.hpp"

using namespace trdk::FrontEnd;
using namespace Detail;

BalanceListView::BalanceListView(QWidget *parent)
    : Base(parent), m_generalContextMenu(this), m_dataContextMenu(this) {
  setWindowTitle(tr("Balances"));
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);
  setAlternatingRowColors(true);
  setSelectionBehavior(SelectItems);
  setSelectionMode(ExtendedSelection);
  setItemDelegate(new BalanceItemDelegate(this));
  setIndentation(10);
  InitContextMenu();
  Verify(connect(
      this, &BalanceListView::doubleClicked,
      [this](const QModelIndex &index) { RequestWalletSettings(index); }));
}

void BalanceListView::InitContextMenu() {
  setContextMenuPolicy(Qt::CustomContextMenu);

  InitGeneralContextMenu(m_generalContextMenu);

  {
    auto &action = *new QAction(tr("Settings"), this);
    Verify(connect(&action, &QAction::triggered, [this]() {
      for (auto &index : selectionModel()->selectedIndexes()) {
        RequestWalletSettings(index);
      }
    }));
    m_dataContextMenu.addAction(&action);
  }
  {
    auto *separator = new QAction(this);
    separator->setSeparator(true);
    m_dataContextMenu.addAction(separator);
  }
  InitGeneralContextMenu(m_dataContextMenu);
}

void BalanceListView::InitGeneralContextMenu(QMenu &menu) {
  {
    auto *action = new QAction(tr("Copy\tCtrl+C"), this);
    action->setShortcut(QKeySequence::Copy);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    Verify(connect(action, &QAction::triggered, this,
                   &BalanceListView::CopySelectedValuesToClipboard));
    menu.addAction(action);
  }
  {
    auto *separator = new QAction(this);
    separator->setSeparator(true);
    menu.addAction(separator);
  }
  {
    auto *action = new QAction(tr("Collapse All"), this);
    Verify(connect(action, &QAction::triggered, this,
                   &BalanceListView::collapseAll));
    menu.addAction(action);
  }
  {
    auto *action = new QAction(tr("Expand All"), this);
    Verify(connect(action, &QAction::triggered, this,
                   &BalanceListView::expandAll));
    menu.addAction(action);
  }

  Verify(connect(this, &BalanceListView::customContextMenuRequested, this,
                 &BalanceListView::ShowContextMenu));
}

void BalanceListView::ShowContextMenu(const QPoint &point) {
  const auto &index = indexAt(point);
  const auto &item = ResolveModelIndexItem<BalanceItem>(index);
  (dynamic_cast<const BalanceDataItem *>(&item) != nullptr
       ? m_dataContextMenu
       : m_generalContextMenu)
      .exec(viewport()->mapToGlobal(point));
}

void BalanceListView::CopySelectedValuesToClipboard() const {
  auto *selection = selectionModel();
  const auto &indexes = selection->selectedIndexes();
  const QModelIndex *previous = nullptr;
  QString result;
  for (const auto &current : indexes) {
    if (previous) {
      current.parent().row() != previous->parent().row() ? result.append('\n')
                                                         : result.append(", ");
    }
    result.append(model()->data(current).toString());
    previous = &current;
  }
  QApplication::clipboard()->setText(result);
}

void BalanceListView::setModel(QAbstractItemModel *model) {
  Base::setModel(model);
  expandAll();
}

void BalanceListView::rowsInserted(const QModelIndex &index,
                                   const int start,
                                   const int end) {
  Base::rowsInserted(index, start, end);
  if (start == 0) {
    expand(index);
  }
  for (auto i = 0; i < header()->count(); ++i) {
    resizeColumnToContents(i);
  }
}

void BalanceListView::RequestWalletSettings(const QModelIndex &index) {
  const auto &item = ResolveModelIndexItem<BalanceItem>(index);
  const auto &symbol = dynamic_cast<const BalanceDataItem *>(&item);
  if (!symbol) {
    return;
  }
  emit WalletSettingsRequested(
      symbol->GetRecord().symbol,
      dynamic_cast<const BalanceTradingSystemItem &>(*item.GetParent())
          .GetTradingSystem());
}
