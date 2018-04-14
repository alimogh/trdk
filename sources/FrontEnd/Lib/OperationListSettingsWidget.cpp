//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "OperationListSettingsWidget.hpp"
#include "GeneratedFiles/ui_OperationListSettingsWidget.h"
#include "OperationListModel.hpp"

using namespace trdk::FrontEnd;

OperationListSettingsWidget::OperationListSettingsWidget(QWidget *parent)
    : Base(parent),
      m_ui(boost::make_unique<Ui::OperationListSettingsWidget>()) {
  m_ui->setupUi(this);
  {
    const auto &now = QDate::currentDate();
    m_ui->filterDateFrom->setDate(now);
    m_ui->filterDateTo->setDate(now);
  }
}

OperationListSettingsWidget::~OperationListSettingsWidget() = default;

void OperationListSettingsWidget::Connect(OperationListModel &model) {
  Verify(connect(m_ui->showTrades, &QCheckBox::toggled, &model,
                 &OperationListModel::ShowTrades));
  Verify(connect(m_ui->showErrors, &QCheckBox::toggled, &model,
                 &OperationListModel::ShowErrors));
  Verify(connect(m_ui->showCancels, &QCheckBox::toggled, &model,
                 &OperationListModel::ShowCancels));
  Verify(connect(m_ui->enableDateFilter, &QCheckBox::toggled,
                 [&model](bool isEnabled) {
                   if (!isEnabled) {
                     model.DisableTimeFilter();
                   }
                 }));
  Verify(
      connect(m_ui->applyDateFilter, &QPushButton::clicked, [this, &model]() {
        model.Filter(m_ui->filterDateFrom->date(), m_ui->filterDateTo->date());
      }));
}
