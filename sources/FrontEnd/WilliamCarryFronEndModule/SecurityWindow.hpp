/*******************************************************************************
 *   Created: 2017/09/17 23:28:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ui_SecurityWindow.h"

namespace trdk {
namespace FrontEnd {
namespace WilliamCarry {

class SecurityWindow : public QMainWindow {
  Q_OBJECT

 public:
  typedef QMainWindow Base;

 public:
  explicit SecurityWindow(const QString &symbol, QWidget *parent);
  virtual ~SecurityWindow() override = default;

 signals:
  void Closed();

 protected:
  virtual void closeEvent(QCloseEvent *) override;

 private:
  Ui::SecurityWindow m_ui;
};
}
}
}