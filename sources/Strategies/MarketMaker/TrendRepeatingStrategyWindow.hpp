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

#include "ui_TrendRepeatingStrategyWindow.h"

namespace trdk {
namespace Strategies {
namespace MarketMaker {

class TrendRepeatingStrategyWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

 public:
  explicit TrendRepeatingStrategyWindow(FrontEnd::Lib::Engine &,
                                        const QString &symbol,
                                        QWidget *parent);
  virtual ~TrendRepeatingStrategyWindow() override;

 protected:
  virtual void closeEvent(QCloseEvent *) override;

 private slots:
  void OnBlocked(const QString &reason);

 signals:
  void Blocked(const QString &reason);

 private:
  void ConnectSignals();
  TrendRepeatingStrategy &CreateStrategy(const QString &symbol);

 private:
  FrontEnd::Lib::Engine &m_engine;
  Ui::TrendRepeatingStrategyWindow m_ui;
  TrendRepeatingStrategy &m_strategy;
};
}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk