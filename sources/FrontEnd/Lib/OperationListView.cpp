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
#include "Engine.hpp"

using namespace trdk::FrontEnd::Lib;

OperationListView::OperationListView(Engine &engine, QWidget *parent)
    : Base(parent), m_engine(engine) {
  setWindowTitle(tr("Operations"));
  setAlternatingRowColors(true);
  setSelectionBehavior(QAbstractItemView::SelectItems);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setContextMenuPolicy(Qt::CustomContextMenu);
  Verify(connect(this, &OperationListView::customContextMenuRequested, this,
                 &OperationListView::ShowContextMenu));
}

void OperationListView::ShowContextMenu(const QPoint &pos) {
  m_contextMenu.exec(mapToGlobal(pos));
}
