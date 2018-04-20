/*******************************************************************************
 *   Created: 2017/11/19 04:56:19
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "ui_TargetTitleWidget.h"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

class TargetTitleWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

  explicit TargetTitleWidget(QWidget *parent);

  void SetTitle(const QString &);
  void Update(const Security &);
  void Update(const Advice &);

 private:
  Ui::TargetTitleWidget m_ui;
  FrontEnd::TimeAdapter<QLabel> m_lastTime;
};
}  // namespace ArbitrageAdvisor
}  // namespace Strategies
}  // namespace trdk