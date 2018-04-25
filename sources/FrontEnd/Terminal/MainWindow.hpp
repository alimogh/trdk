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

#include "Lib/ModuleApi.hpp"
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

  Engine &GetEngine();
  const Engine &GetEngine() const;

  void LoadModule(const boost::filesystem::path &);
  void RestoreModules();

 private:
  void ConnectSignals();

  void CreateModuleWindows(const StrategyWindowFactory &);
  void ShowModuleWindows(StrategyWidgetList &);

  void CreateNewChartWindows();
  void CreateNewStrategyOperationsWindow();
  void CreateNewStandaloneOrderListWindow();
  void CreateNewBalanceListWindow();
  void CreateNewTotalResultsReportWindow();

  Engine &m_engine;
  Ui::MainWindow m_ui;
  std::vector<std::unique_ptr<Lib::Dll>> &m_moduleDlls;
};
}  // namespace Terminal
}  // namespace FrontEnd
}  // namespace trdk