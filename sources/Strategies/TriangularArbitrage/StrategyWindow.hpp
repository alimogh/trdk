/*******************************************************************************
 *   Created: 2018/03/06 12:26:26
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Leg.hpp"
#include "ui_StrategyWindow.h"

namespace trdk {
namespace Strategies {
namespace TriangularArbitrage {

class StrategyWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

 private:
  struct Leg {
    QLabel *symbol;
    QLabel *price;
    QLabel *exchange;
    QFrame *frame;
  };

 public:
  explicit StrategyWindow(FrontEnd::Engine &,
                          const LegsConf &,
                          Context::AddingTransaction &,
                          QWidget *parent);
  explicit StrategyWindow(FrontEnd::Engine &,
                          const QUuid &strategyId,
                          const QString &name,
                          const boost::property_tree::ptree &config,
                          Context::AddingTransaction &,
                          QWidget *parent);
  ~StrategyWindow() override;

  bool IsAutoTradingActivated() const;

 private slots:
  void UpdateOpportunity(const std::vector<Opportunity> &);
  void Block(const QString &reason);

 signals:
  void OpportunityUpdated(const std::vector<Opportunity> &);
  void Blocked(const QString &reason);

 protected:
  void closeEvent(QCloseEvent *) override;

 private:
  void Init(const boost::uuids::uuid &,
            const std::string &name,
            Context::AddingTransaction &);

  void StoreConfig(bool isActive);

  void ConnectSignals();
  Strategy &CreateStrategyInstance(const boost::uuids::uuid &,
                                   const std::string &name,
                                   Context::AddingTransaction &);

  void Disable();

  boost::unordered_set<size_t> GetSelectedStartExchanges() const;
  boost::unordered_set<size_t> GetSelectedMiddleExchanges() const;
  boost::unordered_set<size_t> GetSelectedFinishExchanges() const;
  static boost::unordered_set<size_t> GetSelectedExchanges(
      const QListWidget &, const boost::unordered_set<size_t> &defaultResult);

  void SetSelectedStartExchanges();
  void SetSelectedMiddleExchanges();
  void SetSelectedFinishExchanges();
  void SetSelectedExchanges(QListWidget &,
                            const boost::unordered_set<size_t> &) const;

  FrontEnd::Engine &m_engine;
  boost::property_tree::ptree m_config;
  QString m_investCurrency;
  QString m_resultCurrency;
  Ui::StrategyWindow m_ui{};
  boost::array<Leg, numberOfLegs> m_legs{};

  boost::signals2::scoped_connection m_opportunityUpdateConnection;
  boost::signals2::scoped_connection m_blockConnection;

  int m_maxNumberOfOppotunities = 0;

  Strategy *m_strategy = nullptr;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk
