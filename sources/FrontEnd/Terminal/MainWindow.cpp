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
#include "Charts/CandlestickChartWidget.hpp"
#include "Lib/BalanceListModel.hpp"
#include "Lib/BalanceListView.hpp"
#include "Lib/DropCopy.hpp"
#include "Lib/Engine.hpp"
#include "Lib/OperationListModel.hpp"
#include "Lib/OperationListSettingsWidget.hpp"
#include "Lib/OperationListView.hpp"
#include "Lib/OrderListModel.hpp"
#include "Lib/OrderListView.hpp"
#include "Lib/SortFilterProxyModel.hpp"
#include "Lib/SymbolSelectionDialog.hpp"
#include "Lib/TotalResultsReportModel.hpp"
#include "Lib/TotalResultsReportSettingsWidget.hpp"
#include "Lib/TotalResultsReportView.hpp"

using namespace trdk::Lib;
using namespace trdk::FrontEnd;
using namespace Charts;
using namespace Terminal;

namespace fs = boost::filesystem;

MainWindow::MainWindow(Engine &engine,
                       std::vector<std::unique_ptr<Dll>> &moduleDlls,
                       QWidget *parent)
    : QMainWindow(parent),
      m_engine(engine),
      m_ui({}),
      m_moduleDlls(moduleDlls) {
  m_ui.setupUi(this);
  setWindowTitle(QCoreApplication::applicationName());

  ConnectSignals();

  CreateNewBalanceListWindow();
  CreateNewStrategyOperationsWindow();
  CreateNewStandaloneOrderListWindow();
  CreateNewTotalResultsReportWindow();

  m_ui.centalTabs->setCurrentIndex(0);
}

MainWindow::~MainWindow() = default;

void MainWindow::ConnectSignals() {
  Verify(connect(
      m_ui.centalTabs, &QTabWidget::tabCloseRequested, [this](const int index) {
        std::unique_ptr<const QWidget> widget(m_ui.centalTabs->widget(index));
        try {
          m_ui.centalTabs->removeTab(index);
        } catch (...) {
          widget.release();
          throw;
        }
      }));

#ifdef _DEBUG
  {
    auto &action = *new QAction("Test operation list");
    m_ui.menuHelp->insertAction(m_ui.showAbout, &action);
    m_ui.menuHelp->insertSeparator(m_ui.showAbout);
    Verify(
        connect(&action, &QAction::triggered, [this]() { m_engine.Test(); }));
  }
#endif

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

  Verify(connect(m_ui.createNewChartWindow, &QAction::triggered, this,
                 &MainWindow::CreateNewChartWindow));
  Verify(connect(m_ui.createNewStrategyOperationsWindow, &QAction::triggered,
                 this, &MainWindow::CreateNewStrategyOperationsWindow));
  Verify(connect(m_ui.createNewStandaloneOrdersWindow, &QAction::triggered,
                 this, &MainWindow::CreateNewStandaloneOrderListWindow));
  Verify(connect(m_ui.createNewTotalResultsReportWindow, &QAction::triggered,
                 this, &MainWindow::CreateNewTotalResultsReportWindow));
}

Engine &MainWindow::GetEngine() { return m_engine; }

const Engine &MainWindow::GetEngine() const {
  return const_cast<MainWindow *>(this)->GetEngine();
}

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
  widgetPos->setX(widgetPos->x() + 50);
  widgetPos->setY(widgetPos->y() + 25);
  const auto &screen = QApplication::desktop()->screenGeometry();
  for (auto &widgetPtr : widgets) {
    auto &widget = *widgetPtr;
    widget.resize(widget.minimumSize());
    widget.show();
    widget.setAttribute(Qt::WA_DeleteOnClose);
    widgetPtr.release();
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
      [this, &widgets](const QUuid &typeId, const QUuid &instanceId,
                       const QString &name, const QString &config) {
        for (const auto &module : m_moduleDlls) {
          try {
            for (auto &widget :
                 module->GetFunction<StrategyWidgetList(
                     Engine &, const QUuid &, const QUuid &, const QString &,
                     const QString &, QWidget *)>("RestoreStrategyWidgets")(
                     m_engine, typeId, instanceId, name, config, this)) {
              widgets.emplace_back(widget.release());
            }
          } catch (const Dll::DllFuncException &) {
            continue;
          }
        }
      });
  ShowModuleWindows(widgets);
}

void MainWindow::CreateNewChartWindow() {
  for (const auto &symbol :
       SymbolSelectionDialog(m_engine, this).RequestSymbols()) {
    auto window = boost::make_unique<QMainWindow>(this);
    window->setWindowTitle(tr("%1 Candlestick Chart").arg(symbol));
    {
      auto &widget = *new CandlestickChartWidget(&*window);
      const auto symbolStr = symbol.toStdString();
      Verify(connect(
          &m_engine.GetDropCopy(), &DropCopy::PriceUpdate, &widget,
          [&widget, symbolStr](const Security *security) {
            if (security->GetSymbol().GetSymbol() != symbolStr) {
              return;
            }
            const auto &askPrice = security->GetAskPrice();
            const auto lastPrice =
                askPrice + (std::abs(askPrice - security->GetBidPrice()) / 2);
            widget.OnPriceUpdate(
                ConvertToQDateTime(security->GetLastMarketDataTime()),
                lastPrice);
          },
          Qt::QueuedConnection));
      window->setCentralWidget(&widget);
    }
    window->resize(400, 250);
    window->show();
    window->setAttribute(Qt::WA_DeleteOnClose);
    window.release();
  }
}

void MainWindow::CreateNewStrategyOperationsWindow() {
  auto &tab = *new QWidget(this);
  {
    auto &settings = *new OperationListSettingsWidget(&tab);
    auto &view = *new OperationListView(&tab);
    {
      auto &model = *new OperationListModel(m_engine, &tab);
      view.setModel(&model);
      settings.Connect(model);
    }
    {
      auto &layout = *new QVBoxLayout(&tab);
      tab.setLayout(&layout);
      layout.addWidget(&settings);
      layout.addWidget(&view);
    }
  }
  m_ui.centalTabs->setCurrentIndex(
      m_ui.centalTabs->addTab(&tab, tr("Strategy operations")));
}

void MainWindow::CreateNewStandaloneOrderListWindow() {
  auto &view = *new OrderListView(m_engine, this);
  {
    auto &model = *new SortFilterProxyModel(&view);
    model.setSourceModel(new OrderListModel(m_engine, &view));
    view.setModel(&model);
    Verify(connect(&model, &QAbstractItemModel::rowsInserted,
                   [&view](const QModelIndex &index, int, int) {
                     view.sortByColumn(0, Qt::DescendingOrder);
                     view.scrollTo(index);
                   }));
  }
  m_ui.centalTabs->setCurrentIndex(
      m_ui.centalTabs->addTab(&view, tr("Standalone orders")));
}

void MainWindow::CreateNewBalanceListWindow() {
  auto &view = *new BalanceListView(this);
  {
    auto &model = *new SortFilterProxyModel(&view);
    model.setSourceModel(new BalanceListModel(m_engine, &view));
    view.setModel(&model);
  }
  m_ui.balances->setWidget(&view);
}

void MainWindow::CreateNewTotalResultsReportWindow() {
  auto &tab = *new QWidget(this);
  {
    auto &settings = *new TotalResultsReportSettingsWidget(m_engine, &tab);
    auto &view = *new TotalResultsReportView(&tab);
    {
      auto &model = *new TotalResultsReportModel(m_engine, &tab);
      view.setModel(&model);
      model.Build(settings.GetStartTime(), settings.GetEndTime(),
                  settings.GetStrategy());
      settings.Connect(model);
    }
    {
      auto &layout = *new QVBoxLayout(&tab);
      tab.setLayout(&layout);
      layout.addWidget(&settings);
      layout.addWidget(&view);
    }
  }
  m_ui.centalTabs->setCurrentIndex(
      m_ui.centalTabs->addTab(&tab, tr("Total Results Report")));
}
