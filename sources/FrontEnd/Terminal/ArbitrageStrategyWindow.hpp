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

  struct Target {
    TradingSystem *tradingSystem;
    Security *security;

    mutable Lib::SideAdapter<QLabel> bid;
    mutable Lib::SideAdapter<QLabel> ask;
    mutable Lib::TimeAdapter<QLabel> time;

    const Security *GetSecurityPtr() const { return security; }
  };

  struct BySecurity {};

  typedef boost::multi_index_container<
      Target,
      boost::multi_index::indexed_by<boost::multi_index::hashed_unique<
          boost::multi_index::tag<BySecurity>,
          boost::multi_index::const_mem_fun<Target,
                                            const Security *,
                                            &Target::GetSecurityPtr>>>>
      TargetList;

  struct InstanceData {
    TargetList targets;
    TradingSystem *novaexchangeTradingSystem;
    TradingSystem *yobitnetTradingSystem;
    TradingSystem *ccexTradingSystem;
    TradingSystem *gdaxTradingSystem;
  };

 public:
  explicit ArbitrageStrategyWindow(
      Lib::Engine &,
      MainWindow &mainWindow,
      const boost::optional<QString> &defaultSymbol,
      QWidget *parent);

 private slots:
  void UpdatePrices(const Security *);
  void OnCurrentSymbolChange(int symbolIndex);

 private:
  void LoadSymbols(const boost::optional<QString> &defaultSymbol);
  void SetCurrentSymbol(int symbolIndex);
  void UpdateAllPrices();
  void UpdateTargetPrices(const Target &);

  void Sell(TradingSystem &);
  void Buy(TradingSystem &);

 private:
  const TradingMode m_tradingMode;
  MainWindow &m_mainWindow;
  Ui::ArbitrageStrategyWindow m_ui;
  Lib::Engine &m_engine;
  int m_currentSymbol;
  std::vector<QWidget *> m_novaexchangeWidgets;
  std::vector<QWidget *> m_yobitnetWidgets;
  std::vector<QWidget *> m_ccexWidgets;
  std::vector<QWidget *> m_gdaxWidgets;
  InstanceData m_instanceData;
};
}
}
}