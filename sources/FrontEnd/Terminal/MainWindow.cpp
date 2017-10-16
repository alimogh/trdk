/*******************************************************************************
 *   Created: 2017/10/15 23:16:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MainWindow.hpp"
#include "ArbitrageStrategyWindow.hpp"
#include "Lib/Engine.hpp"
#include "OrderListModel.hpp"
#include "OrderListView.hpp"

using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Terminal;

MainWindow::MainWindow(std::unique_ptr<Engine> &&engine, QWidget *parent)
    : QMainWindow(parent), m_engine(std::move(engine)) {
  m_ui.setupUi(this);
  setWindowTitle(QCoreApplication::applicationName());

  Verify(connect(m_ui.createNewArbitrageStrategy, &QAction::triggered,
                 [this]() { CreateNewArbitrageStrategy(boost::none); }));

  Verify(connect(m_ui.showOrderList, &QAction::triggered, this,
                 &MainWindow::CreateNewOrderView));

  Verify(connect(m_ui.showAbout, &QAction::triggered,
                 [this]() { ShowAbout(*this); }));
  Verify(connect(m_ui.pinToTop, &QAction::triggered,
                 [this](bool pin) { PinToTop(*this, pin); }));

  CreateNewOrderView();
  CreateNewArbitrageStrategy(boost::none);
}

MainWindow::~MainWindow() = default;

void MainWindow::CreateNewArbitrageStrategy(
    const boost::optional<QString> &defaultSymbol) {
  auto *const window =
      new ArbitrageStrategyWindow(*m_engine, *this, defaultSymbol, this);
  window->show();
}

void MainWindow::CreateNewOrderView() {
  auto *const view = new OrderListView(this);
  view->setWindowTitle(tr("Order List"));
  view->setModel(new OrderListModel(view));
  m_ui.area->addSubWindow(view);
  view->show();
}
