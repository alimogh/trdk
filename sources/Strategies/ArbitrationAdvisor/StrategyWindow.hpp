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
#include "TargetActionsWidget.hpp"
#include "TargetSideWidget.hpp"
#include "TargetTitleWidget.hpp"
#include "ui_StrategyWindow.h"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

class StrategyWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

 private:
  struct TargetWidgets;

  struct Target {
    TradingMode tradingMode;
    Security *security;
    TargetWidgets *widgets;

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
    TradingSystem *bittrexTradingSystem;
    TradingSystem *novaexchangeTradingSystem;
    TradingSystem *yobitnetTradingSystem;
    TradingSystem *ccexTradingSystem;
    TradingSystem *gdaxTradingSystem;
    TradingSystem *bestBuyTradingSystem;
    TradingSystem *bestSellTradingSystem;
  };

  struct TargetWidgets {
    TradingSystem *InstanceData::*tradingSystemField;
    TargetTitleWidget title;
    TargetBidWidget bid;
    TargetAskWidget ask;
    TargetActionsWidget actions;

    explicit TargetWidgets(TradingSystem *InstanceData::*tradingSystemField,
                           QWidget *parent)
        : tradingSystemField(tradingSystemField),
          title(parent),
          bid(parent),
          ask(parent),
          actions(parent) {}

    template <typename Source>
    void Update(const Source &source) {
      title.Update(source);
      bid.Update(source);
      ask.Update(source);
    }
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

  void AddTarget(const QString &name,
                 const std::string &targetId,
                 TradingSystem *InstanceData::*tradingSystemField);

 private:
  const TradingMode m_tradingMode;
  const size_t m_instanceNumber;
  boost::unordered_map<std::string, boost::uuids::uuid> m_strategiesUuids;
  Ui::StrategyWindow m_ui;
  FrontEnd::Lib::Engine &m_engine;
  int m_symbol;
  FrontEnd::Lib::PriceAdapter<QLabel> m_bestSpreadAbsValue;
  boost::unordered_map<std::string, std::unique_ptr<TargetWidgets>>
      m_targetWidgets;
  InstanceData m_instanceData;
};
}
}
}