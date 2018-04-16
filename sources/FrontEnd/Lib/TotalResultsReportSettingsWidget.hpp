//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "Api.h"
#include "Fwd.hpp"

namespace Ui {
class TotalResultsReportSettingsWidget;
}

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API TotalResultsReportSettingsWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

  explicit TotalResultsReportSettingsWidget(const Engine &engine,
                                            QWidget *parent);
  ~TotalResultsReportSettingsWidget();

  void Connect(TotalResultsReportModel &);

  QDateTime GetStartTime() const;
  boost::optional<QDateTime> GetEndTime() const;

 private:
  std::unique_ptr<Ui::TotalResultsReportSettingsWidget> m_ui;
};
}  // namespace FrontEnd
}  // namespace trdk