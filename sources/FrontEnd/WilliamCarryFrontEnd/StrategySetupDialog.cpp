/*******************************************************************************
 *   Created: 2017/10/15 15:56:13
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "StrategySetupDialog.hpp"

using namespace trdk;
using namespace trdk::FrontEnd::WilliamCarry;

StrategySetupDialog::StrategySetupDialog(QWidget *parent) : QDialog(parent) {
  m_ui.setupUi(this);
  ActualizeStrategiesTitle();

  Verify(connect(m_ui.strategy1, &QCheckBox::stateChanged,
                 [this]() { ActualizeStrategyTitle(1, *m_ui.strategy1); }));
  Verify(connect(m_ui.strategy2, &QCheckBox::stateChanged,
                 [this]() { ActualizeStrategyTitle(2, *m_ui.strategy2); }));
  Verify(connect(m_ui.strategy3, &QCheckBox::stateChanged,
                 [this]() { ActualizeStrategyTitle(3, *m_ui.strategy3); }));
  Verify(connect(m_ui.strategy4, &QCheckBox::stateChanged,
                 [this]() { ActualizeStrategyTitle(4, *m_ui.strategy4); }));
}

void StrategySetupDialog::ActualizeStrategiesTitle() {
  ActualizeStrategyTitle(1, *m_ui.strategy1);
  ActualizeStrategyTitle(2, *m_ui.strategy2);
  ActualizeStrategyTitle(3, *m_ui.strategy3);
  ActualizeStrategyTitle(4, *m_ui.strategy4);
}

void StrategySetupDialog::ActualizeStrategyTitle(int strategyNumber,
                                                 const QCheckBox &enabled) {
  AssertLt(0, strategyNumber);
  m_ui.strategiesPages->setItemText(
      strategyNumber - 1,
      enabled.isChecked()
          ? QString(tr("Strategy %1").arg(strategyNumber))
          : QString(tr("Strategy %1 (disabled)").arg(strategyNumber)));
}
