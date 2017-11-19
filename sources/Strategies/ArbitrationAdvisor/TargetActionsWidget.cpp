/*******************************************************************************
*   Created: 2017/11/19 12:18:21
*    Author: Eugene V. Palchukovsky
*    E-mail: eugene@palchukovsky.com
* -------------------------------------------------------------------
*   Project: Trading Robot Development Kit
*       URL: http://robotdk.com
* Copyright: Eugene V. Palchukovsky
******************************************************************************/

#include "Prec.hpp"
#include "TargetActionsWidget.hpp"

using namespace trdk;
using namespace trdk::Strategies::ArbitrageAdvisor;

TargetActionsWidget::TargetActionsWidget(QWidget *parent) : Base(parent) {
  m_ui.setupUi(this);
  Verify(connect(m_ui.sell, &QPushButton::clicked,
                 [this]() { emit Order(ORDER_SIDE_SELL); }));
  Verify(connect(m_ui.buy, &QPushButton::clicked,
                 [this]() { emit Order(ORDER_SIDE_BUY); }));
}
