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
class OperationListSettingsWidget;
}

namespace trdk {
namespace FrontEnd {

class TRDK_FRONTEND_LIB_API OperationListSettingsWidget : public QWidget {
  Q_OBJECT

 public:
  typedef QWidget Base;

  explicit OperationListSettingsWidget(QWidget *parent);
  ~OperationListSettingsWidget();

  void Connect(OperationListModel &);

 private:
  std::unique_ptr<Ui::OperationListSettingsWidget> m_ui;
};
}  // namespace FrontEnd
}  // namespace trdk