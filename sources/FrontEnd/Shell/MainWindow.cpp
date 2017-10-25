/*******************************************************************************
 *   Created: 2017/09/05 08:18:37
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MainWindow.hpp"
#include "EngineListModel.hpp"
#include "EngineWindow.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Shell;

MainWindow::MainWindow(QWidget *parent)
    : Base(parent),
      m_engineListModel(boost::make_unique<EngineListModel>(
          GetExeFilePath().branch_path() / "etc", this)) {
  m_ui.setupUi(this);
  m_ui.engineList->setModel(&*m_engineListModel);
  setWindowTitle(QCoreApplication::applicationName() + " " +
                 TRDK_BUILD_IDENTITY);

  Verify(connect(m_ui.showAbout, &QAction::triggered,
                 [this]() { ShowAbout(*this); }));
  Verify(connect(m_ui.engineList, &QListView::clicked, this,
                 &MainWindow::ShowEngine));
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *closeEvent) {
  Base::closeEvent(closeEvent);
  QApplication::quit();
}

void MainWindow::ShowEngine(const QModelIndex &index) {
  auto &engine = m_engineListModel->GetEngine(index);
  engine.show();
  engine.activateWindow();
}
