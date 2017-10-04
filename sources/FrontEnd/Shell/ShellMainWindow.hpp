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

#include "ui_ShellMainWindow.h"

namespace trdk {
namespace FrontEnd {
namespace Shell {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

 public:
  explicit MainWindow(QWidget *parent);
  virtual ~MainWindow() override;

 protected:
  virtual void closeEvent(QCloseEvent *) override;

 private slots:
  void ShowAboutInfo();
  void ShowEngine(const QModelIndex &);

 private:
  Ui::MainWindow m_ui;
  std::unique_ptr<EngineListModel> m_engineListModel;
};
}
}
}