/*******************************************************************************
 *   Created: 2018/01/24 23:17:47
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

class TRDK_FRONTEND_LIB_API OperationListView : public QTreeView {
  Q_OBJECT

 public:
  typedef QTreeView Base;

 public:
  explicit OperationListView(Engine &, QWidget *parent);

 public slots:
  void ShowContextMenu(const QPoint &);

 private:
  Engine &m_engine;
  QMenu m_contextMenu;
};
}  // namespace Lib
}  // namespace FrontEnd
}  // namespace trdk
