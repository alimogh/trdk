/*******************************************************************************
 *   Created: 2017/10/04 22:34:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TimersDialog.hpp"
#include "Core/Security.hpp"

using namespace trdk;
using namespace trdk::FrontEnd::WilliamCarry;

TimersDialog::TimersDialog(QWidget *parent) : QDialog(parent) {
  m_ui.setupUi(this);
}

void TimersDialog::Add(const Security &security) {
  m_ui.currency1->addItem(
      QString::fromStdString(security.GetSymbol().GetSymbol()));
  m_ui.currency2->addItem(
      QString::fromStdString(security.GetSymbol().GetSymbol()));
  m_ui.currency3->addItem(
      QString::fromStdString(security.GetSymbol().GetSymbol()));
  m_ui.currency4->addItem(
      QString::fromStdString(security.GetSymbol().GetSymbol()));
  m_ui.currency5->addItem(
      QString::fromStdString(security.GetSymbol().GetSymbol()));
}
