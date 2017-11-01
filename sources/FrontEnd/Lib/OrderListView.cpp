/*******************************************************************************
 *   Created: 2017/10/16 21:41:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OrderListView.hpp"

using namespace trdk::FrontEnd::Lib;

OrderListView::OrderListView(Engine &engine, QWidget *parent)
    : Base(parent), m_engine(engine) {
  setWindowTitle(tr("Order List"));
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);
  setAlternatingRowColors(true);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);

  m_contextMenu.addAction(tr("&Cancel"), this, &OrderListView::OnCancelRequest);

  setContextMenuPolicy(Qt::CustomContextMenu);
  Verify(connect(this, &OrderListView::customContextMenuRequested, this,
                 &OrderListView::ShowContextMenu));
}

void OrderListView::ShowContextMenu(const QPoint &pos) {
  m_contextMenu.exec(mapToGlobal(pos));
}

void OrderListView::OnCancelRequest() {}