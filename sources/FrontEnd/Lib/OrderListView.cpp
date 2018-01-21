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
#include "Core/Context.hpp"
#include "Engine.hpp"

using namespace trdk::FrontEnd::Lib;

OrderListView::OrderListView(Engine &engine, QWidget *parent)
    : Base(parent), m_engine(engine) {
  setWindowTitle(tr("Order List"));
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);
  setAlternatingRowColors(true);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  verticalHeader()->setVisible(false);

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
  const size_t tradingSystemIndex =
      item.data(ITEM_DATA_ROLE_TRADING_SYSTEM_INDEX).toULongLong();
  const TradingMode mode =
      static_cast<TradingMode>(item.data(ITEM_DATA_ROLE_TRADING_MODE).toInt());
  TradingSystem &tradingSystem =
      m_engine.GetContext().GetTradingSystem(tradingSystemIndex, mode);
  try {
    if (!tradingSystem.CancelOrder(orderId)) {
      QMessageBox::warning(
          this, tr("Order cancel"),
          tr("%1 does not have order in the active list.")
              .arg(
                  QString::fromStdString(tradingSystem.GetInstanceName()),  // 1
                  QString::fromStdString(orderId.GetValue())),              // 2
          QMessageBox::Cancel);
      return false;
    }
  } catch (const std::exception &ex) {
    QMessageBox::critical(
        this, tr("Order cancel"),
        tr("Failed to cancel order %1 at the exchange %2: \"%3\".")
            .arg(QString::fromStdString(orderId.GetValue()),               // 1
                 QString::fromStdString(tradingSystem.GetInstanceName()),  // 2
                 ex.what()),                                               // 3
        QMessageBox::Cancel);
    return false;
  }
  return true;
}