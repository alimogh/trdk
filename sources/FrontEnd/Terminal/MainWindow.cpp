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
#include "Lib/BalanceListModel.hpp"
#include "Lib/Engine.hpp"
#include "Lib/ModuleApi.hpp"
#include "Lib/OperationListModel.hpp"
#include "Lib/OrderListModel.hpp"
#include "Lib/SortFilterProxyModel.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Terminal;

namespace fs = boost::filesystem;
namespace lib = trdk::FrontEnd::Lib;

MainWindow::MainWindow(lib::Engine &engine,
                       std::vector<std::unique_ptr<trdk::Lib::Dll>> &moduleDlls,
                       QWidget *parent)
    : QMainWindow(parent),
      m_engine(engine),
      m_operationListView(this),
      m_standaloneOrderList(m_engine, this),
      m_balanceList(this),
      m_moduleDlls(moduleDlls) {
  m_ui.setupUi(this);
  setWindowTitle(QCoreApplication::applicationName());

#ifdef DEV_VER
  {
    auto &action = *new QAction("Test operation list");
    m_ui.mainMenu->addAction(&action);
    Verify(
        connect(&action, &QAction::triggered, [this]() { m_engine.Test(); }));
  }
#endif  // DEV_VER

  {
    auto *model = new OperationListModel(m_engine, &m_operationListView);
    m_operationListView.setModel(model);
    auto *layout = new QVBoxLayout;
    layout->addWidget(&m_operationListView);
    m_ui.operations->setLayout(layout);
  }

  {
    auto *model = new SortFilterProxyModel(&m_standaloneOrderList);
    model->setSourceModel(new OrderListModel(m_engine, &m_standaloneOrderList));
    m_standaloneOrderList.setModel(model);
    Verify(connect(model, &QAbstractItemModel::rowsInserted,
                   [this](const QModelIndex &index, int, int) {
                     m_standaloneOrderList.sortByColumn(0, Qt::DescendingOrder);
                     m_standaloneOrderList.scrollTo(index);
                   }));
    auto *layout = new QVBoxLayout;
    layout->addWidget(&m_standaloneOrderList);
    m_ui.standaloneOrders->setLayout(layout);
  }

  {
    auto *model = new SortFilterProxyModel(&m_balanceList);
    model->setSourceModel(new BalanceListModel(m_engine, &m_balanceList));
    m_balanceList.setModel(model);
    m_ui.balances->setWidget(&m_balanceList);
  }

  Verify(connect(&m_engine, &Lib::Engine::Message,
                 [this](const QString &message, bool isCritical) {
                   if (isCritical) {
                     QMessageBox::critical(this, tr("Engine critical warning"),
                                           message, QMessageBox::Ok);
                   } else {
                     QMessageBox::information(this, tr("Engine information"),
                                              message, QMessageBox::Ok);
                   }
                 }));

  Verify(connect(m_ui.showAbout, &QAction::triggered,
                 [this]() { ShowAbout(*this); }));
}

MainWindow::~MainWindow() = default;

void MainWindow::LoadModule(const fs::path &path) {
  try {
    m_moduleDlls.emplace_back(std::make_unique<Dll>(path, true));
  } catch (const Dll::Error &ex) {
    qDebug() << "Failed to load module: \"" << ex.what() << "\".";
    return;
  }
  StrategyMenuActionList actions;
  try {
    actions =
        m_moduleDlls.back()->GetFunction<StrategyMenuActionList(lib::Engine &)>(
            "CreateMenuActions")(m_engine);
  } catch (const std::exception &ex) {
    const auto &error =
        QString("Failed to load module front-end: \"%1\".").arg(ex.what());
    QMessageBox::critical(this, tr("Failed to load module."), error,
                          QMessageBox::Ignore);
  }
  for (auto &actionDesc : actions) {
    const auto &callback = actionDesc.second;
    auto &action = *new QAction(actionDesc.first, this);
    m_ui.strategiesMenu->addAction(&action);
    Verify(connect(&action, &QAction::triggered, [this, callback]() {
      for (auto &widgetPtr : callback(this)) {
        auto &widget = *widgetPtr.release();
        widget.adjustSize();
        widget.show();
      }
    }));
  }
}
