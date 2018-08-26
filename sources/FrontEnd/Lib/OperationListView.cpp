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
#include "OperationItemDelegate.hpp"

using namespace trdk::FrontEnd;

OperationListView::OperationListView(QWidget *parent)
    : Base(parent),
      m_isFollowingEnabled(true),
      m_isExpandingEnabled(true),
      m_numberOfResizesForOperations(),
      m_numberOfResizesForOrder(0) {
  setAlternatingRowColors(true);
  setSelectionBehavior(SelectItems);
  setSelectionMode(ExtendedSelection);
  setItemDelegate(new OperationItemDelegate(this));
  setIndentation(10);
  InitContextMenu();
}

void OperationListView::InitContextMenu() {
  setContextMenuPolicy(Qt::ActionsContextMenu);
  {
    auto *action = new QAction(tr("Copy\tCtrl+C"), this);
    action->setShortcut(QKeySequence::Copy);
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    Verify(connect(action, &QAction::triggered, this,
                   &OperationListView::CopySelectedValuesToClipboard));
    addAction(action);
  }
  {
    auto *separator = new QAction(this);
    separator->setSeparator(true);
    addAction(separator);
  }
  {
    auto *action = new QAction(tr("Follow New Operations\tF"), this);
    action->setShortcut(QKeySequence(Qt::Key_F));
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    action->setCheckable(true);
    action->setChecked(m_isFollowingEnabled);
    Verify(connect(action, &QAction::toggled, this,
                   &OperationListView::FollowNewRecords));
    addAction(action);
  }
  {
    auto *action = new QAction(tr("Expand New Operations"), this);
    action->setCheckable(true);
    action->setChecked(m_isExpandingEnabled);
    Verify(connect(
        action, &QAction::toggled, this,
        [this](const bool isEnabled) { m_isExpandingEnabled = isEnabled; }));
    addAction(action);
  }
  {
    auto separator = new QAction(this);
    separator->setSeparator(true);
    addAction(separator);
  }
  {
    auto *action = new QAction(tr("Collapse All"), this);
    Verify(connect(action, &QAction::triggered, this,
                   &OperationListView::collapseAll));
    addAction(action);
  }
  {
    auto *action = new QAction(tr("Expand All"), this);
    Verify(connect(action, &QAction::triggered, this,
                   &OperationListView::expandAll));
    addAction(action);
  }
}

void OperationListView::CopySelectedValuesToClipboard() {
  auto *selection = selectionModel();
  auto indexes = selection->selectedIndexes();
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
                                     const int start,
                                     const int end) {
  Base::rowsInserted(index, start, end);
  if (start == 0) {
    if (m_numberOfResizesForOperations < 3) {
      for (auto i = 0; i < header()->count(); ++i) {
        resizeColumnToContents(i);
      }
      ++m_numberOfResizesForOperations;
    }
  } else if (m_numberOfResizesForOrder < 3) {
    expand(index);
    for (auto i = 0; i < header()->count(); ++i) {
      resizeColumnToContents(i);
    }
    ++m_numberOfResizesForOrder;
  }
  if (m_isFollowingEnabled) {
    ScrollToLastChild(*this, index);
  }
  m_isExpandingEnabled ? expand(index) : collapse(index);
}

void OperationListView::FollowNewRecords(const bool isEnabled) {
  m_isFollowingEnabled = isEnabled;
  if (!m_isFollowingEnabled) {
    return;
  }
  ScrollToLastChild(*this);
}
