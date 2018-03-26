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
#include "Lib/OperationListModel.hpp"
#include "Lib/OrderListModel.hpp"
#include "Lib/SortFilterProxyModel.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Terminal;

namespace fs = boost::filesystem;
namespace ids = boost::uuids;

MainWindow::MainWindow(Engine &engine,
                       std::vector<std::unique_ptr<Dll>> &moduleDlls,
                       QWidget *parent)
    : QMainWindow(parent),
      m_engine(engine),
      m_operationListView(this),
      m_standaloneOrderList(m_engine, this),
      m_balanceList(this),
      m_moduleDlls(moduleDlls) {
  m_ui.setupUi(this);
  setWindowTitle(QCoreApplication::applicationName());

#ifdef _DEBUG
  {
    auto &action = *new QAction("Test operation list");
    m_ui.menuHelp->insertAction(m_ui.showAbout, &action);
    m_ui.menuHelp->insertSeparator(m_ui.showAbout);
    Verify(
        connect(&action, &QAction::triggered, [this]() { m_engine.Test(); }));
  }
#endif  // DEV_VER

  {
    auto *model = new OperationListModel(m_engine, &m_operationListView);

    m_operationListView.setModel(model);
    m_ui.verticalLayout->addWidget(&m_operationListView);

    m_ui.operationsFilterDateFrom->setDate(QDate::currentDate());
    m_ui.operationsFilterDateTo->setDate(QDate::currentDate());

    Verify(connect(m_ui.showTradeOperations, &QCheckBox::toggled, model,
                   &OperationListModel::IncludeTrades));
    Verify(connect(m_ui.showErrorOperations, &QCheckBox::toggled, model,
                   &OperationListModel::IncludeErrors));
    Verify(connect(m_ui.showCanceledOperations, &QCheckBox::toggled, model,
                   &OperationListModel::IncludeCancels));

    Verify(connect(m_ui.enableOperationsDateFilter, &QCheckBox::toggled,
                   [model](bool isEnabled) {
                     if (!isEnabled) {
                       model->DisableTimeFilter();
                     }
                   }));
    Verify(connect(m_ui.applyOperationsDateFilter, &QPushButton::clicked,
                   [this, model]() {
                     model->Filter(m_ui.operationsFilterDateFrom->date(),
                                   m_ui.operationsFilterDateTo->date());
                   }));
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

  Verify(connect(&m_engine, &Engine::Message,
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
        m_moduleDlls.back()->GetFunction<StrategyMenuActionList(Engine &)>(
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
    Verify(connect(&action, &QAction::triggered,
                   [this, callback]() { CreateModuleWindows(callback); }));
  }
}

void MainWindow::CreateModuleWindows(const StrategyWindowFactory &factory) {
  auto widgets = factory(this);
  ShowModuleWindows(widgets);
}

void MainWindow::ShowModuleWindows(StrategyWidgetList &widgets) {
  boost::optional<QPoint> widgetPos = pos();
  boost::optional<QSize> size;
  widgetPos->setX(widgetPos->x() + 25);
  widgetPos->setY(widgetPos->y() + 25);
  const auto &screen = QApplication::desktop()->screenGeometry();
  for (auto &widgetPtr : widgets) {
    auto &widget = *widgetPtr.release();
    widget.resize(widget.minimumSize());
    widget.show();
    if (!widgetPos) {
      continue;
    }
    widget.move(*widgetPos);
    if (!size) {
      size = widget.size();
    }
    widgetPos->setX(widget.pos().x() + size->width() + 10);
    if (widgetPos->x() + size->width() > screen.width()) {
      widgetPos->setX(pos().x() + 25);
      widgetPos->setY(widget.pos().y() + size->height() + 10);
      if (widgetPos->y() + size->height() > screen.height()) {
        widgetPos = boost::none;
      }
    }
  }
}

void MainWindow::RestoreModules() {
  StrategyWidgetList widgets;
  m_engine.ForEachActiveStrategy(
      [this, &widgets](const QUuid &id, const QString &config) {
        for (const auto &module : m_moduleDlls) {
          try {
            for (auto &widget :
                 module->GetFunction<StrategyWidgetList(
                     Engine &, const QUuid &, const QString &, QWidget *)>(
                     "RestoreStrategyWidgets")(m_engine, id, config, this)) {
              widgets.emplace_back(widget.release());
            }
          } catch (const Dll::DllFuncException &) {
            continue;
          }
        }
      });
  ShowModuleWindows(widgets);
}
