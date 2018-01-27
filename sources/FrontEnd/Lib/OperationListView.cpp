/*******************************************************************************
 *   Created: 2018/01/24 23:18:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OperationListView.hpp"
#include "OpeartionItemDelegate.hpp"

using namespace trdk::FrontEnd::Lib;

OperationListView::OperationListView(QWidget *parent) : Base(parent) {
  setAlternatingRowColors(true);
  setSelectionBehavior(QAbstractItemView::SelectItems);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setItemDelegate(new OpeartionItemDelegate(this));
  InitContextMenu();
}

void OperationListView::InitContextMenu() {
  setContextMenuPolicy(Qt::ActionsContextMenu);
  {
    auto *action = new QAction(tr("&Copy\tCtrl+C"), this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    Verify(connect(action, &QAction::triggered, this,
                   &OperationListView::CopySelectedValuesToClipboard));
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
                   &OperationListView::collapseAll));
    addAction(action);
  }
  {
    auto *action = new QAction(tr("&Expand All"), this);
    Verify(connect(action, &QAction::triggered, this,
                   &OperationListView::expandAll));
    addAction(action);
  }
}

void OperationListView::CopySelectedValuesToClipboard() {
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

void OperationListView::rowsInserted(const QModelIndex &index,
                                     int start,
                                     int end) {
  if (index.isValid() && !index.parent().isValid()) {
    expand(index);
  }
  Base::rowsInserted(index, start, end);
}
