/*******************************************************************************
 *   Created: 2017/10/16 00:42:57
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ui_ArbitrageStrategyWindow.h"

namespace trdk {
namespace FrontEnd {
namespace Terminal {

class ArbitrageStrategyWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit ArbitrageStrategyWindow(QWidget *parent);

 private:
  Ui::ArbitrageStrategyWindow m_ui;
};
}
}
}