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
#include "Engine.hpp"

using namespace trdk::FrontEnd;

OrderListView::OrderListView(Engine &engine, QWidget *parent)
    : Base(parent), m_engine(engine) {
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);
  setAlternatingRowColors(true);
  setSelectionBehavior(SelectRows);
  setSelectionMode(ExtendedSelection);
  verticalHeader()->setVisible(false);
  verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  verticalHeader()->setDefaultSectionSize(
      verticalHeader()->fontMetrics().height() + 4);

  m_contextMenu.addAction(tr("&Cancel"), this,
                          &OrderListView::CancelSelectedOrders);

  setContextMenuPolicy(Qt::CustomContextMenu);
  Verify(connect(this, &OrderListView::customContextMenuRequested, this,
                 &OrderListView::ShowContextMenu));
}

void OrderListView::ShowContextMenu(const QPoint &pos) {
  m_contextMenu.exec(mapToGlobal(pos));
}

void OrderListView::CancelSelectedOrders() {
  for (const auto &item : selectionModel()->selectedRows()) {
    if (!CancelOrder(item)) {
      break;
    }
  }
}

bool OrderListView::CancelOrder(const QModelIndex &item) {
  const OrderId orderId(
      item.data(ITEM_DATA_ROLE_ITEM_ID).toString().toStdString());
  const auto tradingSystemIndex =
      item.data(ITEM_DATA_ROLE_TRADING_SYSTEM_INDEX).toULongLong();
  const auto mode =
      static_cast<TradingMode>(item.data(ITEM_DATA_ROLE_TRADING_MODE).toInt());
  auto &tradingSystem =
      m_engine.GetContext().GetTradingSystem(tradingSystemIndex, mode);
  try {
    if (!tradingSystem.CancelOrder(orderId)) {
      QMessageBox::warning(
          this, tr("Order cancel"),
          tr("%1 does not have order in the active list.")
              .arg(QString::fromStdString(tradingSystem.GetTitle()),  // 1
                   QString::fromStdString(orderId.GetValue())),       // 2
          QMessageBox::Cancel);
      return false;
    }
  } catch (const std::exception &ex) {
    QMessageBox::critical(
        this, tr("Order cancel"),
        tr("Failed to cancel order %1 at the exchange %2: \"%3\".")
            .arg(QString::fromStdString(orderId.GetValue()),        // 1
                 QString::fromStdString(tradingSystem.GetTitle()),  // 2
                 ex.what()),                                        // 3
        QMessageBox::Cancel);
    return false;
  }
  return true;
}