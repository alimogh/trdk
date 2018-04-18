/*******************************************************************************
 *   Created: 2018/02/20 00:09:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ui_TakerStrategyWindow.h"

namespace trdk {
namespace Strategies {
namespace MarketMaker {

class TakerStrategyWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

  explicit TakerStrategyWindow(FrontEnd::Engine &,
                               const QString &symbol,
                               QWidget *parent);
  ~TakerStrategyWindow();

 private slots:
  void OnCompleted();
  void OnBlocked(const QString &reason);
  void OnVolumeUpdate(const Volume &currentVolume, const Volume &maxVolume);
  void OnPnlUpdate(const Volume &currentPnl, const Volume &maxLoss);
  void OnStrategyEvent(const QString &);

 signals:
  void Completed();
  void Blocked(const QString &reason);
  void VolumeUpdate(const Volume &currentVolume, const Volume &maxVolume);
  void PnlUpdate(const Volume &currentPnl, const Volume &maxLoss);
  void StrategyEvent(const QString &);

 private:
  bool LoadExchanges();
  void ConnectSignals();
  TakerStrategy &CreateStrategyInstance(const QString &symbol);

  void Disable();

  FrontEnd::Engine &m_engine;
  Ui::TakerStrategyWindow m_ui;
  bool m_hasExchanges;

  boost::signals2::scoped_connection m_completedConnection;
  boost::signals2::scoped_connection m_blockConnection;
  boost::signals2::scoped_connection m_volumeUpdateConnection;
  boost::signals2::scoped_connection m_pnlUpdateConnection;
  boost::signals2::scoped_connection m_eventsConnection;

  TakerStrategy &m_strategy;
};
}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk