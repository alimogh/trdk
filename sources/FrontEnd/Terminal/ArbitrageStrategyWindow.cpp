/*******************************************************************************
 *   Created: 2017/10/16 00:42:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "ArbitrageStrategyWindow.hpp"

using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Terminal;

ArbitrageStrategyWindow::ArbitrageStrategyWindow(QWidget *parent)
    : QMainWindow(parent) {
  m_ui.setupUi(this);
  setWindowTitle(tr("Arbitrage") + " - " + QCoreApplication::applicationName());
  setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
                 Qt::WindowSystemMenuHint);
}
