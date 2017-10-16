/*******************************************************************************
 *   Created: 2017/10/15 21:12:10
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
namespace FrontEnd {
namespace Terminal {

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(std::unique_ptr<Lib::Engine> &&, QWidget *parent);
  ~MainWindow() override;

 public slots:
  void CreateNewArbitrageStrategy();
  void CreateNewOrderView();

 private:
  std::unique_ptr<Lib::Engine> m_engine;
  Ui::MainWindow m_ui;
};
}
}
}