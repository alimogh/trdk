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

#include "Lib/BalanceListView.hpp"
#include "Lib/OperationListView.hpp"
#include "Lib/OrderListView.hpp"
#include "ui_MainWindow.h"

namespace trdk {
namespace FrontEnd {
namespace Terminal {

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(Lib::Engine &,
                      std::vector<std::unique_ptr<trdk::Lib::Dll>> &moduleDlls,
                      QWidget *parent);
  ~MainWindow() override;

 public:
  Lib::Engine &GetEngine() { return m_engine; }

 public slots:
  void CreateNewArbitrageStrategy();

 private:
  Lib::Engine &m_engine;
  Ui::MainWindow m_ui;
  Lib::OperationListView m_operationListView;
  Lib::OrderListView m_standaloneOrderList;
  Lib::BalanceListView m_balanceList;
  std::vector<std::unique_ptr<trdk::Lib::Dll>> &m_moduleDlls;
};
}  // namespace Terminal
}  // namespace FrontEnd
}  // namespace trdk