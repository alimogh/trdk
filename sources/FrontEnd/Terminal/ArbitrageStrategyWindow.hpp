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
  typedef QMainWindow Base;

 private:
  struct Target {
    TradingSystem *tradingSystem;
    Security *security;

    mutable Lib::SideAdapter<QLabel> bid;
    mutable Lib::SideAdapter<QLabel> ask;
    mutable Lib::TimeAdapter<QLabel> time;

    QFrame *bidFrame;
    QFrame *askFrame;

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
  virtual ~ArbitrageStrategyWindow() override;

 public:
  virtual QSize sizeHint() const override;

 protected:
  virtual void closeEvent(QCloseEvent *) override;

 private slots:
  void UpdatePrices(const Security *);
  void OnCurrentSymbolChange(int symbolIndex);
  void HighlightPrices();

 private:
  void LoadSymbols(const boost::optional<QString> &defaultSymbol);
  void SetCurrentSymbol(int symbolIndex);
  void UpdateAllPrices();
  void UpdateTargetPrices(const Target &);

  void Sell(TradingSystem &);
  void Buy(TradingSystem &);

 private:
  const TradingMode m_tradingMode;
  const size_t m_instanceNumber;
  boost::unordered_map<std::string, boost::uuids::uuid> m_strategiesUuids;
  MainWindow &m_mainWindow;
  Ui::ArbitrageStrategyWindow m_ui;
  Lib::Engine &m_engine;
  Strategy *m_currentStrategy;
  int m_currentSymbol;
  Lib::PriceAdapter<QLabel> m_bestSpreadAbsValue;
  std::vector<QWidget *> m_novaexchangeWidgets;
  std::vector<QWidget *> m_yobitnetWidgets;
  std::vector<QWidget *> m_ccexWidgets;
  std::vector<QWidget *> m_gdaxWidgets;
  InstanceData m_instanceData;
};
}
}
}