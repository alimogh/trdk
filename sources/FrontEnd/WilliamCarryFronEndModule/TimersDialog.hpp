/*******************************************************************************
 *   Created: 2017/10/04 22:31:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "Prec.hpp"
#include "GeneratedFiles/ui_TimersDialog.h"

namespace trdk {
namespace FrontEnd {
namespace WilliamCarry {

class TimersDialog : public QDialog {
  Q_OBJECT
 public:
  explicit TimersDialog(QWidget *parent);

 public:
  void Add(const trdk::Security &);

 private:
  Ui::TimersDialog m_ui;
};
}
}
}