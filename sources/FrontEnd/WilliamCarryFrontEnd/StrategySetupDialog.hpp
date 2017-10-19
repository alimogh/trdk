/*******************************************************************************
 *   Created: 2017/10/15 12:49:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "GeneratedFiles/ui_StrategySetupDialog.h"

namespace trdk {
namespace FrontEnd {
namespace WilliamCarry {

class StrategySetupDialog : public QDialog {
  Q_OBJECT
 public:
  explicit StrategySetupDialog(QWidget *parent);

 private:
  void ActualizeStrategiesTitle();
  void ActualizeStrategyTitle(int strategyNumber, const QCheckBox &enabled);

 private:
  Ui::StrategySetupDialog m_ui;
};
}
}
}