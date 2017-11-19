/*******************************************************************************
 *   Created: 2017/11/19 11:21:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "ui_TargetActionsWidget.h"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

class TargetActionsWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

 public:
  explicit TargetActionsWidget(QWidget *parent);

 signals:
  void Order(const OrderSide &);

 private:
  Ui::TargetActionsWidget m_ui;
};
}
}
}