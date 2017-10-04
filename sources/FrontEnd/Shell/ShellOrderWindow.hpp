/*******************************************************************************
 *   Created: 2017/09/30 04:35:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ShellFwd.hpp"
#include "ui_ShellOrderWindow.h"

namespace trdk {
namespace FrontEnd {
namespace Shell {

class OrderWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

 public:
  explicit OrderWindow(Engine &, QWidget *parent);
  virtual ~OrderWindow() override = default;

 public:
  void SetSecurity(Security &);
  TradingSystem *GetSelectedTradingSystem();
  TradingMode GetSelectedTradingMode() const;

 signals:
  void Closed();

 protected:
  virtual void closeEvent(QCloseEvent *) override;

 private slots:
  void LoadTradingSystemList(int index);
  void OnStateChanged(bool isStarted);
  void UpdatePrices(const Security *);

  void SendBuyOrder();
  void SendSellOrder();

  void SelectTradingSystem();

 private:
  Ui::OrderWindow m_ui;
  Engine &m_engine;
  Security *m_security;
};
}
}
}