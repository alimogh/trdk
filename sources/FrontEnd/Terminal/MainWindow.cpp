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
#include "Lib/Engine.hpp"
#include "Lib/OrderListModel.hpp"
#include "Lib/SortFilterProxyModel.hpp"
#include "Lib/Util.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Terminal;

MainWindow::MainWindow(Engine &engine,
                       std::vector<std::unique_ptr<trdk::Lib::Dll>> &moduleDlls,
                       QWidget *parent)
    : QMainWindow(parent),
      m_engine(engine),
      m_orderList(m_engine, this),
      m_moduleDlls(moduleDlls) {
  m_ui.setupUi(this);
  setWindowTitle(QCoreApplication::applicationName());

  {
    auto *model = new SortFilterProxyModel(&m_orderList);
    model->setSourceModel(new OrderListModel(m_engine, &m_orderList));
    m_orderList.setModel(model);
    Verify(connect(model, &QAbstractItemModel::rowsInserted,
                   [this](const QModelIndex &index, int, int) {
                     m_orderList.sortByColumn(0, Qt::DescendingOrder);
                     m_orderList.scrollTo(index);
                   }));
  }
  m_ui.area->addSubWindow(&m_orderList);

  Verify(connect(m_ui.createNewArbitrageStrategy, &QAction::triggered, this,
                 &MainWindow::CreateNewArbitrageStrategy));

  Verify(connect(m_ui.showOrderList, &QAction::triggered, [this]() {
    m_orderList.activateWindow();
    m_orderList.showNormal();
  }));

  Verify(connect(m_ui.showAbout, &QAction::triggered,
                 [this]() { ShowAbout(*this); }));
  Verify(connect(m_ui.pinToTop, &QAction::triggered,
                 [this](bool pin) { PinToTop(*this, pin); }));

  try {
    m_moduleDlls.emplace_back(
        std::make_unique<Dll>("ArbitrationAdvisor", true));
  } catch (const std::exception &ex) {
    const auto &error =
        QString("Failed to load module: \"%1\".").arg(ex.what());
    QMessageBox::critical(this, tr("Failed to load module."), error,
                          QMessageBox::Ignore);
    throw;
  }

  m_orderList.showMaximized();
}

MainWindow::~MainWindow() = default;

void MainWindow::CreateNewArbitrageStrategy() {
  try {
    typedef std::unique_ptr<QWidget>(Factory)(Lib::Engine &, QWidget *);
    auto *const widget =
        m_moduleDlls.back()
            ->GetFunction<Factory>("CreateStrategyWidgets")(m_engine, this)
            .release();
    widget->adjustSize();
    widget->show();
  } catch (const std::exception &ex) {
    const auto &error =
        QString("Failed to load module front-end: \"%1\".").arg(ex.what());
    QMessageBox::critical(this, tr("Failed to load module."), error,
                          QMessageBox::Ignore);
  }
}
