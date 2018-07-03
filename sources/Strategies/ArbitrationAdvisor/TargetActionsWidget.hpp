/*******************************************************************************
 *   Created: 2017/11/19 11:21:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ui_TargetActionsWidget.h"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

class TargetActionsWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

  explicit TargetActionsWidget(QWidget *parent);

 signals:
  void Buy();
  void Sell();

 private:
  Ui::TargetActionsWidget m_ui;
};
}  // namespace ArbitrageAdvisor
}  // namespace Strategies
}  // namespace trdk