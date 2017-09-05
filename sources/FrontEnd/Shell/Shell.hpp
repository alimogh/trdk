/*******************************************************************************
 *   Created: 2017/09/05 08:25:07
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ui_Shell.h"

class Shell : public QMainWindow {
  Q_OBJECT

 public:
  explicit Shell(QWidget *parent = Q_NULLPTR);

 private slots:
  void ShowAboutInfo();

 private:
  Ui::ShellClass ui;
};
