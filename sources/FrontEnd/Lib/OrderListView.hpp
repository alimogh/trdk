/*******************************************************************************
 *   Created: 2017/10/16 21:40:46
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Api.h"
#include "Fwd.hpp"

namespace trdk {
namespace FrontEnd {
namespace Lib {

class TRDK_FRONTEND_LIB_API OrderListView : public QTableView {
  Q_OBJECT

 public:
  typedef QTableView Base;

 public:
  explicit OrderListView(Engine &, QWidget *parent);

 public slots:
  void ShowContextMenu(const QPoint &);

 private:
  void CancelSelectedOrders();
  bool CancelOrder(const QModelIndex &);

 private:
  Engine &m_engine;
  QMenu m_contextMenu;
};
}
}
}