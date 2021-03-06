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
    Security* security;
    TargetWidgets* widgets;

    const Security* GetSecurityPtr() const { return security; }
    const std::string& GetSymbol() const;
    const TradingSystem* GetTradingSystem() const;
  };

  struct BySecurity {};

  struct BySymbol {};

  typedef boost::multi_index_container<
      Target,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<
              boost::multi_index::tag<BySecurity>,
              boost::multi_index::const_mem_fun<Target,
                                                const Security*,
                                                &Target::GetSecurityPtr>>,
          boost::multi_index::hashed_unique<
              boost::multi_index::tag<BySymbol>,
              boost::multi_index::composite_key<
                  Target,
                  boost::multi_index::const_mem_fun<Target,
                                                    const TradingSystem*,
                                                    &Target::GetTradingSystem>,
                  boost::multi_index::const_mem_fun<Target,
                                                    const std::string&,
                                                    &Target::GetSymbol>>>>>
      TargetList;

  struct TargetWidgets {
    TargetTitleWidget title;
    TargetBidWidget bid;
    TargetAskWidget ask;
    TargetActionsWidget actions;

    explicit TargetWidgets(const QString& title, QWidget* parent)
        : title(parent), bid(parent), ask(parent), actions(parent) {
      this->title.SetTitle(title);
    }

    template <typename Source>
    void Update(const Source& source) {
      title.Update(source);
      bid.Update(source);
      ask.Update(source);
    }
  };

 public:
  explicit StrategyWindow(FrontEnd::Engine&,
                          const QString& symbol,
                          Context::AddingTransaction&,
                          QWidget* parent);
  explicit StrategyWindow(FrontEnd::Engine&,
                          const QUuid& strategyId,
                          const QString& name,
                          const boost::property_tree::ptree& config,
                          Context::AddingTransaction&,
                          QWidget* parent);
  ~StrategyWindow() override;

  QSize sizeHint() const override;

 protected:
  void closeEvent(QCloseEvent*) override;

 private slots:
  void TakeAdvice(const trdk::Strategies::ArbitrageAdvisor::Advice&);
  void OnSignalCheckErrors(const std::vector<std::string>&);
  void OnBlocked(const QString& reason);

  void ToggleAutoTrading(bool activate);
  void DeactivateAutoTrading();
  void UpdateAutoTradingLevel(const Lib::Double& level, const Qty& maxQty);

  void UpdateAdviceLevel(double level);

 signals:
  void Advice(const trdk::Strategies::ArbitrageAdvisor::Advice&);
  void SignalCheckErrors(const std::vector<std::string>&);
  void Blocked(const QString& reason);

 private:
  void Init();

  void ConnectSignals();
  void SendOrder(const OrderSide&, TradingSystem*);

  bool IsAutoTradingActivated() const;

  Strategy& GenerateNewStrategyInstance(Context::AddingTransaction&);
  Strategy& RestoreStrategyInstance(const QUuid& strategyId,
                                    const std::string& name,
                                    const boost::property_tree::ptree& config,
                                    Context::AddingTransaction&);
  Strategy& CreateStrategyInstance(const boost::uuids::uuid& strategyId,
                                   const std::string& name,
                                   const boost::property_tree::ptree& config,
                                   Context::AddingTransaction&);

  void AddTargetWidgets(TargetWidgets&);

  boost::property_tree::ptree CreateConfig(
      const boost::uuids::uuid& strategyId,
      const Lib::Double& minPriceDifferenceToHighlightPercentage,
      bool isAutoTradingEnabled,
      const Lib::Double& minPriceDifferenceToTradePercentage,
      const Qty& maxQty,
      bool isLowestSpreadEnabled,
      const Lib::Double& lowestSpreadPercentage) const;
  boost::property_tree::ptree DumpConfig() const;

  void StoreConfig(bool isActive);

  Ui::StrategyWindow m_ui;
  boost::unordered_map<TradingSystem*, std::unique_ptr<TargetWidgets>>
      m_targetWidgets;
  FrontEnd::PriceAdapter<QLabel> m_bestSpreadAbsValue;

  FrontEnd::Engine& m_engine;
  const TradingMode m_tradingMode;
  const std::string m_symbol;

  Strategy& m_strategy;
  boost::signals2::scoped_connection m_adviceConnection;
  boost::signals2::scoped_connection m_tradingSignalCheckErrorsConnection;
  boost::signals2::scoped_connection m_blockConnection;

  TargetList m_targets;
  TradingSystem* m_bestBuyTradingSystem;
  TradingSystem* m_bestSellTradingSystem;
};
}  // namespace ArbitrageAdvisor
}  // namespace Strategies
}  // namespace trdk
