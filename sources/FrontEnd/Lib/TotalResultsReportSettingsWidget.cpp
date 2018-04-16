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
#include "TotalResultsReportSettingsWidget.hpp"
#include "Engine.hpp"
#include "GeneratedFiles/ui_TotalResultsReportSettingsWidget.h"
#include "TotalResultsReportModel.hpp"

using namespace trdk::FrontEnd;

TotalResultsReportSettingsWidget::TotalResultsReportSettingsWidget(
    const Engine &engine, QWidget *parent)
    : Base(parent),
      m_ui(boost::make_unique<Ui::TotalResultsReportSettingsWidget>()) {
  m_ui->setupUi(this);
  {
    const auto &now = QDateTime::currentDateTime();
    m_ui->startTime->setDateTime(now);
    m_ui->endTime->setDateTime(now);
  }
  Verify(connect(&engine, &Engine::OperationUpdate,
                 [this](const Orm::Operation &) {}));
}

TotalResultsReportSettingsWidget::~TotalResultsReportSettingsWidget() = default;

void TotalResultsReportSettingsWidget::Connect(TotalResultsReportModel &model) {
  Verify(connect(m_ui->apply, &QPushButton::clicked, [this, &model]() {
    model.Build(GetStartTime(), GetEndTime());
  }));
}

QDateTime TotalResultsReportSettingsWidget::GetStartTime() const {
  return m_ui->startTime->dateTime();
}

boost::optional<QDateTime> TotalResultsReportSettingsWidget::GetEndTime()
    const {
  if (!m_ui->enableEndTime->isChecked()) {
    return boost::none;
  }
  return m_ui->endTime->dateTime();
}
