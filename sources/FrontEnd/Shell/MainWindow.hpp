/*******************************************************************************
 *   Created: 2017/09/05 08:25:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ui_MainWindow.h"

namespace trdk {
namespace Frontend {
namespace Shell {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = Q_NULLPTR);

 private slots:
  void ShowAboutInfo();
  void ShowEngine(const QModelIndex &);

 private:
  Ui::MainWindowClass ui;

  EngineListModel *m_engineListModel;
};
}
}
}