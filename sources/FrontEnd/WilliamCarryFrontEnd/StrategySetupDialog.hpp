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
#include "StrategySettings.hpp"

namespace trdk {
namespace FrontEnd {
namespace WilliamCarry {

class StrategySetupDialog : public QDialog {
  Q_OBJECT
 private:
  struct Fields {
    QCheckBox *enabled;

    QSpinBox *lotMultiplier;

    QRadioButton *target1;
    QRadioButton *target2;
    QRadioButton *targetGrid5;
    QRadioButton *targetGrid10;

    QSpinBox *target1Pips;
    QSpinBox *target1Time;
    QSpinBox *target2Pips;
    QSpinBox *target2Time;

    QSpinBox *stopLossStage1Pips;
    QCheckBox *stopLossStage2Enabled;
    QSpinBox *stopLossStage2Pips;
    QSpinBox *stopLossStage2Time;
    QCheckBox *stopLossStage3Enabled;
    QSpinBox *stopLossStage3Pips;
    QSpinBox *stopLossStage3Time;

    QCheckBox *broker1;
    QCheckBox *broker2;
    QCheckBox *broker3;
    QCheckBox *broker4;
    QCheckBox *broker5;

    void Import(const StrategySettings &);
    StrategySettings Export() const;
  };

 public:
  explicit StrategySetupDialog(
      const boost::array<StrategySettings, NUMBER_OF_STRATEGIES> &,
      QWidget *parent);

 public:
  boost::array<StrategySettings, NUMBER_OF_STRATEGIES> GetSettings() const;

 private:
  void ActualizeStrategiesTitle();
  void ActualizeStrategyTitle(int strategyNumber, const QCheckBox &enabled);

 private:
  Ui::StrategySetupDialog m_ui;
  boost::array<Fields, NUMBER_OF_STRATEGIES> m_fields;
};
}
}
}