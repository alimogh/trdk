/*******************************************************************************
 *   Created: 2017/11/19 04:59:45
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TargetTitleWidget.hpp"
#include "Advice.hpp"

using namespace trdk;
using namespace trdk::Strategies::ArbitrageAdvisor;

TargetTitleWidget::TargetTitleWidget(QWidget *parent) : Base(parent) {
  m_ui.setupUi(this);
  m_lastTime = FrontEnd::TimeAdapter<QLabel>(*m_ui.lastTime);
}

void TargetTitleWidget::SetTitle(const QString &title) {
  m_ui.title->setText(title);
}

void TargetTitleWidget::Update(const Security &security) {
  m_lastTime.Set(security.GetLastMarketDataTime());
}

void TargetTitleWidget::Update(const Advice &advice) {
  m_lastTime.Set(advice.time);
}
