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
#include "Lib/ModuleApi.hpp"
#include "Lib/OperationListView.hpp"
#include "Lib/OrderListView.hpp"
#include "ui_MainWindow.h"

namespace trdk {
namespace FrontEnd {
namespace Terminal {

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(Engine &,
                      std::vector<std::unique_ptr<Lib::Dll>> &moduleDlls,
                      QWidget *parent);
  ~MainWindow() override;

  Engine &GetEngine() { return m_engine; }

  void LoadModule(const boost::filesystem::path &);
  void RestoreModules();

 private:
  void CreateModuleWindows(const StrategyWindowFactory &);
  void ShowModuleWindows(StrategyWidgetList &);

  Engine &m_engine;
  Ui::MainWindow m_ui;
  OperationListView m_operationListView;
  OrderListView m_standaloneOrderList;
  BalanceListView m_balanceList;
  std::vector<std::unique_ptr<Lib::Dll>> &m_moduleDlls;
};
}  // namespace Terminal
}  // namespace FrontEnd
}  // namespace trdk