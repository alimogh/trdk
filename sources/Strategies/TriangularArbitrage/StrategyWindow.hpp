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
  explicit StrategyWindow(FrontEnd::Lib::Engine &,
                          const LegsConf &,
                          QWidget *parent);
  ~StrategyWindow();

 private slots:
  void OnOpportunityUpdate(const std::vector<Opportunity> &);
  void OnTradingSignalCheckErrors(const std::vector<std::string> &);
  void OnBlocked(const QString &reason);

 signals:
  void OpportunityUpdated(const std::vector<Opportunity> &);
  void TradingSignalCheckErrors(const std::vector<std::string> &);
  void Blocked(const QString &reason);

 private:
  void ConnectSignals();
  Strategy &CreateStrategyInstance(const LegsConf &);

  void Disable();

  void SetSelectedStartExchange();
  void SetSelectedMiddleExchange();
  void SetSelectedFinishExchange();
  void SetSelectedExchange(QComboBox &, const boost::optional<size_t> &) const;

 private:
  FrontEnd::Lib::Engine &m_engine;
  const QString m_startCurrency;
  Ui::StrategyWindow m_ui;
  boost::array<Leg, numberOfLegs> m_legs;

  boost::signals2::scoped_connection m_opportunityUpdateConnection;
  boost::signals2::scoped_connection m_tradingSignalCheckErrorsConnection;
  boost::signals2::scoped_connection m_blockConnection;

  size_t m_maxNumberOfOppotunities;

  Strategy &m_strategy;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk
