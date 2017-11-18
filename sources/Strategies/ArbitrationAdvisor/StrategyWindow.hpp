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

#include "Advice.hpp"
#include "ui_StrategyWindow.h"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

class StrategyWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

 private:
  struct Target {
    TradingMode tradingMode;

    Security *security;

    mutable FrontEnd::Lib::SideAdapter<QLabel> bid;
    mutable FrontEnd::Lib::SideAdapter<QLabel> ask;
    mutable FrontEnd::Lib::TimeAdapter<QLabel> time;

    QFrame *bidFrame;
    QFrame *askFrame;

    const Security *GetSecurityPtr() const { return security; }
    const std::string &GetSymbol() const;
    const trdk::TradingSystem *GetTradingSystem() const;
  };

  struct BySecurity {};
  struct BySymbol {};

  typedef boost::multi_index_container<
      Target,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<
              boost::multi_index::tag<BySecurity>,
              boost::multi_index::const_mem_fun<Target,
                                                const Security *,
                                                &Target::GetSecurityPtr>>,
          boost::multi_index::hashed_unique<
              boost::multi_index::tag<BySymbol>,
              boost::multi_index::composite_key<
                  Target,
                  boost::multi_index::const_mem_fun<Target,
                                                    const TradingSystem *,
                                                    &Target::GetTradingSystem>,
                  boost::multi_index::const_mem_fun<Target,
                                                    const std::string &,
                                                    &Target::GetSymbol>>>>>
      TargetList;

  struct InstanceData {
    Strategy *strategy;
    boost::signals2::scoped_connection adviceConnection;
    TargetList targets;
    TradingSystem *novaexchangeTradingSystem;
    TradingSystem *yobitnetTradingSystem;
    TradingSystem *ccexTradingSystem;
    TradingSystem *gdaxTradingSystem;
    TradingSystem *bestBuyTradingSystem;
    TradingSystem *bestSellTradingSystem;
  };

 public:
  explicit StrategyWindow(FrontEnd::Lib::Engine &,
                          const boost::optional<QString> &defaultSymbol,
                          QWidget *parent);
  virtual ~StrategyWindow() override;

 public:
  virtual QSize sizeHint() const override;

 protected:
  virtual void closeEvent(QCloseEvent *) override;

 private slots:
  void TakeAdvice(const trdk::Strategies::ArbitrageAdvisor::Advice &);
  void OnCurrentSymbolChange(int symbolIndex);

  void ToggleAutoTrading(bool activate);
  void DeactivateAutoTrading();
  void UpdateAutoTradingLevel(double level);

  void UpdateAdviceLevel(double level);

 signals:
  void Advice(const trdk::Strategies::ArbitrageAdvisor::Advice &);

 private:
  void LoadSymbols(const boost::optional<QString> &defaultSymbol);
  void SetCurrentSymbol(int symbolIndex);

  void SendOrder(const OrderSide &, TradingSystem *);

  bool IsAutoTradingActivated() const;

 private:
  const TradingMode m_tradingMode;
  const size_t m_instanceNumber;
  boost::unordered_map<std::string, boost::uuids::uuid> m_strategiesUuids;
  Ui::StrategyWindow m_ui;
  FrontEnd::Lib::Engine &m_engine;
  int m_symbol;
  FrontEnd::Lib::PriceAdapter<QLabel> m_bestSpreadAbsValue;
  std::vector<QWidget *> m_novaexchangeWidgets;
  std::vector<QWidget *> m_yobitnetWidgets;
  std::vector<QWidget *> m_ccexWidgets;
  std::vector<QWidget *> m_gdaxWidgets;
  InstanceData m_instanceData;
};
}
}
}