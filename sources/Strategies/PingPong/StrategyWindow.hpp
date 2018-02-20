/*******************************************************************************
 *   Created: 2018/01/31 21:34:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ui_StrategyWindow.h"

namespace trdk {
namespace Strategies {
namespace PingPong {

class StrategyWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

 public:
  explicit StrategyWindow(FrontEnd::Lib::Engine &,
                          const QString &symbol,
                          QWidget *parent);
  ~StrategyWindow();

 private slots:
  void OnBlocked(const QString &reason);
  void OnStrategyEvent(const QString &);

 signals:
  void Blocked(const QString &reason);
  void StrategyEvent(const QString &);

 private:
  bool LoadExchanges();
  void ConnectSignals();
  Strategy &CreateStrategyInstance(const QString &symbol);

  void Disable();

 private:
  FrontEnd::Lib::Engine &m_engine;
  Ui::StrategyWindow m_ui;
  bool m_hasExchanges;

  boost::signals2::scoped_connection m_blockConnection;
  boost::signals2::scoped_connection m_eventsConnection;

  Strategy &m_strategy;
};
}  // namespace PingPong
}  // namespace Strategies
}  // namespace trdk