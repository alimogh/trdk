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
#include "Engine.hpp"

using namespace trdk::FrontEnd::Lib;

BalanceListView::BalanceListView(Engine &engine, QWidget *parent)
    : Base(parent), m_engine(engine) {
  setWindowTitle(tr("Balances"));
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);
  setAlternatingRowColors(true);
  setSelectionBehavior(QAbstractItemView::SelectItems);
  setSelectionMode(QAbstractItemView::ExtendedSelection);

  //   m_contextMenu.addAction(tr("&Copy"), this,
  //                           &BalanceListView::CopySelectedValues);

  setContextMenuPolicy(Qt::CustomContextMenu);
  Verify(connect(this, &BalanceListView::customContextMenuRequested, this,
                 &BalanceListView::ShowContextMenu));
}

void BalanceListView::ShowContextMenu(const QPoint &pos) {
  m_contextMenu.exec(mapToGlobal(pos));
}
