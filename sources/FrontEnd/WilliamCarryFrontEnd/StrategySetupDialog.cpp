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
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::WilliamCarry;

namespace pt = boost::posix_time;

#define InitFields(numb)                                                      \
  m_fields[numb - 1].enabled = m_ui.strategy##numb;                           \
  m_fields[numb - 1].lotMultiplier = m_ui.strategy##numb##LotMultiplier;      \
  m_fields[numb - 1].target1 = m_ui.strategy##numb##Target1;                  \
  m_fields[numb - 1].target2 = m_ui.strategy##numb##Target2;                  \
  m_fields[numb - 1].targetGrid5 = m_ui.strategy##numb##TargetGrid5;          \
  m_fields[numb - 1].targetGrid10 = m_ui.strategy##numb##TargetGrid10;        \
  m_fields[numb - 1].target1Pips = m_ui.strategy##numb##Target1Value;         \
  m_fields[numb - 1].target1Time = m_ui.strategy##numb##Target1Time;          \
  m_fields[numb - 1].target2Pips = m_ui.strategy##numb##Target2Value;         \
  m_fields[numb - 1].target2Time = m_ui.strategy##numb##Target2Time;          \
  m_fields[numb - 1].stopLossStage1Pips = m_ui.strategy##numb##StopLoss1Pips; \
  m_fields[numb - 1].stopLossStage2Enabled = m_ui.strategy##numb##StopLoss2;  \
  m_fields[numb - 1].stopLossStage2Pips = m_ui.strategy##numb##StopLoss2Pips; \
  m_fields[numb - 1].stopLossStage2Time = m_ui.strategy##numb##StopLoss2Time; \
  m_fields[numb - 1].stopLossStage3Enabled = m_ui.strategy##numb##StopLoss3;  \
  m_fields[numb - 1].stopLossStage3Pips = m_ui.strategy##numb##StopLoss3Pips; \
  m_fields[numb - 1].stopLossStage3Time = m_ui.strategy##numb##StopLoss3Time; \
  m_fields[numb - 1].broker1 = m_ui.strategy##numb##Broker1;                  \
  m_fields[numb - 1].broker2 = m_ui.strategy##numb##Broker2;                  \
  m_fields[numb - 1].broker3 = m_ui.strategy##numb##Broker3;                  \
  m_fields[numb - 1].broker4 = m_ui.strategy##numb##Broker4;                  \
  m_fields[numb - 1].broker5 = m_ui.strategy##numb##Broker5;                  \
  m_fields[numb - 1].Import(settings[numb - 1])

void StrategySetupDialog::Fields::Import(const StrategySettings &source) {
  enabled->setChecked(source.isEnabled);
  if (!source.isEnabled) {
    return;
  }

  lotMultiplier->setValue(source.lotMultiplier);

  target1Pips->setValue(static_cast<int>(source.takeProfit1.pips));
  target1Time->setValue(source.takeProfit1.delay.total_seconds());

  switch (source.numberOfStepsToTarget) {
    default:
      AssertEq(1, source.numberOfStepsToTarget);
    case 1:
      if (!source.takeProfit2) {
        target1->setChecked(true);
      } else {
        target2->setChecked(true);
        target2Pips->setValue(static_cast<int>(source.takeProfit2->pips));
        target2Time->setValue(source.takeProfit2->delay.total_seconds());
      }
      break;
    case 5:
      targetGrid5->setChecked(true);
      break;
    case 10:
      targetGrid10->setChecked(true);
      break;
  }

  stopLossStage2Enabled->setChecked(source.stopLoss2 ? true : false);
  if (source.stopLoss2) {
    stopLossStage2Pips->setValue(static_cast<int>(source.stopLoss2->pips));
    stopLossStage2Time->setValue(source.stopLoss2->delay.total_seconds());
  }

  stopLossStage3Enabled->setChecked(source.stopLoss3 ? true : false);
  if (source.stopLoss3) {
    stopLossStage3Pips->setValue(static_cast<int>(source.stopLoss3->pips));
    stopLossStage3Time->setValue(source.stopLoss3->delay.total_seconds());
  }
}

StrategySettings StrategySetupDialog::Fields::Export() const {
  StrategySettings result = {};

  result.isEnabled = enabled->isChecked();

  result.lotMultiplier = static_cast<unsigned int>(lotMultiplier->value());

  result.takeProfit1 = {static_cast<size_t>(target1Pips->value()),
                        pt::seconds(target1Time->value())};

  if (target2->isChecked()) {
    result.takeProfit2 =
        StrategySettings::Target{static_cast<size_t>(target2Pips->value()),
                                 pt::seconds(target2Time->value())};
    result.numberOfStepsToTarget = 1;
  } else if (targetGrid5->isChecked()) {
    result.numberOfStepsToTarget = 5;
  } else if (targetGrid10->isChecked()) {
    result.numberOfStepsToTarget = 10;
  } else {
    Assert(target1->isChecked());
    result.numberOfStepsToTarget = 1;
  }

  if (stopLossStage2Enabled->isChecked()) {
    result.stopLoss2 = StrategySettings::Target{
        static_cast<size_t>(stopLossStage2Pips->value()),
        pt::seconds(stopLossStage2Time->value())};
  }
  if (stopLossStage3Enabled->isChecked()) {
    result.stopLoss3 = StrategySettings::Target{
        static_cast<size_t>(stopLossStage3Pips->value()),
        pt::seconds(stopLossStage3Time->value())};
  }

  return result;
}

StrategySetupDialog::StrategySetupDialog(
    const boost::array<StrategySettings, 4> &settings, QWidget *parent)
    : QDialog(parent) {
  m_ui.setupUi(this);

  InitFields(1);
  InitFields(2);
  InitFields(3);
  InitFields(4);
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

boost::array<StrategySettings, NUMBER_OF_STRATEGIES>
StrategySetupDialog::GetSettings() const {
  return {m_fields[0].Export(), m_fields[1].Export(), m_fields[2].Export(),
          m_fields[3].Export()};
}
