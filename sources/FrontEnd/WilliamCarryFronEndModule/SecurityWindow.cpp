/*******************************************************************************
 *   Created: 2017/09/17 23:22:51
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "SecurityWindow.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::WilliamCarry;

SecurityWindow::SecurityWindow(const QString &symbol, QWidget *parent)
    : Base(parent) {
  m_ui.setupUi(this);
  m_ui.symbol->setText(symbol);
  m_ui.lastTime->setText("00:00:00");
  m_ui.bidPrice->setText("0.00000");
  m_ui.spread->setText("0.00000");
  m_ui.askPrice->setText("0.00000");
  setWindowTitle(symbol + " - " + QCoreApplication::applicationName());
}

void SecurityWindow::closeEvent(QCloseEvent *closeEvent) {
  Base::closeEvent(closeEvent);
  emit Closed();
}
