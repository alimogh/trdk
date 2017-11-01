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
#include "Lib/OrderListModel.hpp"
#include "Lib/SortFilterProxyModel.hpp"

using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Terminal;

MainWindow::MainWindow(std::unique_ptr<Engine> &&engine, QWidget *parent)
    : QMainWindow(parent),
      m_engine(std::move(engine)),
      m_orderList(*m_engine, this) {
  m_ui.setupUi(this);
  setWindowTitle(QCoreApplication::applicationName());

  {
    auto *model = new SortFilterProxyModel(&m_orderList);
    model->setSourceModel(new OrderListModel(*m_engine, &m_orderList));
    m_orderList.setModel(model);
  }
  m_ui.area->addSubWindow(&m_orderList);

  Verify(connect(m_ui.createNewArbitrageStrategy, &QAction::triggered,
                 [this]() { CreateNewArbitrageStrategy(boost::none); }));

  Verify(connect(m_ui.showOrderList, &QAction::triggered, [this]() {
    m_orderList.activateWindow();
    m_orderList.showNormal();
  }));

  Verify(connect(m_ui.showAbout, &QAction::triggered,
                 [this]() { ShowAbout(*this); }));
  Verify(connect(m_ui.pinToTop, &QAction::triggered,
                 [this](bool pin) { PinToTop(*this, pin); }));

  m_orderList.showMaximized();
}

MainWindow::~MainWindow() = default;

void MainWindow::CreateNewArbitrageStrategy(
    const boost::optional<QString> &defaultSymbol) {
  auto *const window =
      new ArbitrageStrategyWindow(*m_engine, *this, defaultSymbol, this);
  window->show();
}
