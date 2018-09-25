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
#include "Lib/WalletsConfig.hpp"
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

 public slots:
  void ShowRequestedStrategy(const QString &title,
                             const QString &module,
                             const QString &factoryName,
                             const QString &params);
  void ShowWalletSettings(QString symbol, const TradingSystem &);
  void ShowWalletDepositDialog(QString symbol, const TradingSystem &);
  void ReloadWalletsConfig();
  void RechargeWallet(const TradingSystem *,
                      const std::string &symbol,
                      const Volume &available,
                      const Volume &locked);
  void RechargeWallets();

 signals:
  void WalletsRecharged(const TradingSystem *,
                        const QString &symbol,
                        const QDateTime &);

 private slots:
  void StoreWalletRecharingLastTime(const TradingSystem *,
                                    const QString &symbol,
                                    const QDateTime &);

 private:
  void ConnectSignals();

  void CreateModuleWindows(const StrategyWindowFactory &);
  void ShowModuleWindows(StrategyWidgetList &);

  void InitBalanceListWindow();
  void InitSecurityListWindow();
  void InitMarketScannerWindow();

  void CreateNewChartWindows();
  void CreateNewChartWindow(const QString &symbol);
  void CreateNewStrategyOperationsWindow();
  void CreateNewStandaloneOrderListWindow();
  void CreateNewTotalResultsReportWindow();
  void CreateNewOrderWindows(Security &);

  void EditExchangeList();
  void AddDefaultSymbol();

  void StoreWalletsConfig(const boost::property_tree::ptree &);

  Engine &m_engine;
  Ui::MainWindow m_ui;
  std::vector<std::unique_ptr<Lib::Dll>> &m_moduleDlls;

  boost::unordered_map<const Security *, QWidget *> m_orderWindows;

  WalletsConfig m_walletsConfig{m_engine};

  TimerScope m_timerScope;
  QTimer m_timer{this};
};
}  // namespace Terminal
}  // namespace FrontEnd
}  // namespace trdk