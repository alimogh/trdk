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
#include "BalanceItemDelegate.hpp"

using namespace trdk::FrontEnd::Lib;

BalanceListView::BalanceListView(QWidget *parent) : Base(parent) {
  setWindowTitle(tr("Balances"));
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);
  setAlternatingRowColors(true);
  setSelectionBehavior(QAbstractItemView::SelectItems);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setItemDelegate(new BalanceItemDelegate(this));
  setIndentation(10);
  InitContextMenu();
}

void BalanceListView::InitContextMenu() {
  setContextMenuPolicy(Qt::ActionsContextMenu);
  {
    auto *action = new QAction(tr("&Copy\tCtrl+C"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    Verify(connect(action, &QAction::triggered, this,
                   &BalanceListView::CopySelectedValuesToClipboard));
    addAction(action);
  }
  {
    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    addAction(separator);
  }
  {
    auto *action = new QAction(tr("C&ollapse All"), this);
    Verify(connect(action, &QAction::triggered, this,
                   &BalanceListView::collapseAll));
    addAction(action);
  }
  {
    auto *action = new QAction(tr("&Expand All"), this);
    Verify(connect(action, &QAction::triggered, this,
                   &BalanceListView::expandAll));
    addAction(action);
  }
}

void BalanceListView::CopySelectedValuesToClipboard() {
  QItemSelectionModel *selection = selectionModel();
  QModelIndexList indexes = selection->selectedIndexes();
  const QModelIndex *previous = nullptr;
  QString result;
  for (const auto &current : indexes) {
    if (previous) {
      current.row() != previous->row() ? result.append('\n')
                                       : result.append(", ");
    }
    result.append(model()->data(current).toString());
    previous = &current;
  }
  QApplication::clipboard()->setText(result);
}

void BalanceListView::rowsInserted(const QModelIndex &index,
                                   int start,
                                   int end) {
  Base::rowsInserted(index, start, end);
  if (start == 0) {
    expand(index);
    for (int i = 0; i < header()->count(); ++i) {
      resizeColumnToContents(i);
    }
  }
}
