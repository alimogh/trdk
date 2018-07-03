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
#include "Charts/ChartToolbarWidget.hpp"
#include "Lib/BalanceListModel.hpp"
#include "Lib/BalanceListView.hpp"
#include "Lib/Engine.hpp"
#include "Lib/OperationListModel.hpp"
#include "Lib/OperationListSettingsWidget.hpp"
#include "Lib/OperationListView.hpp"
#include "Lib/OrderListModel.hpp"
#include "Lib/OrderListView.hpp"
#include "Lib/OrderWindow.hpp"
#include "Lib/SecurityListModel.hpp"
#include "Lib/SecurityListView.hpp"
#include "Lib/SortFilterProxyModel.hpp"
#include "Lib/SourcePropertiesDialog.hpp"
#include "Lib/SourcesListWidget.hpp"
#include "Lib/SymbolDialog.hpp"
#include "Lib/SymbolSelectionDialog.hpp"
#include "Lib/TotalResultsReportModel.hpp"
#include "Lib/TotalResultsReportSettingsWidget.hpp"
#include "Lib/TotalResultsReportView.hpp"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;
using namespace Charts;
using namespace Terminal;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;
namespace fs = boost::filesystem;

MainWindow::MainWindow(FrontEnd::Engine &engine,
                       std::vector<std::unique_ptr<Dll>> &moduleDlls,
                       QWidget *parent)
    : QMainWindow(parent),
      m_engine(engine),
      m_ui({}),
      m_moduleDlls(moduleDlls) {
  m_ui.setupUi(this);
  setWindowTitle(QCoreApplication::applicationName());

  InitBalanceListWindow();
  InitSecurityListWindow();

  CreateNewStrategyOperationsWindow();
  CreateNewStandaloneOrderListWindow();
  CreateNewTotalResultsReportWindow();

  m_ui.centalTabs->setCurrentIndex(0);

  ConnectSignals();
}

MainWindow::~MainWindow() = default;

void MainWindow::ConnectSignals() {
  Verify(connect(m_ui.editExchangeList, &QAction::triggered, this,
                 &MainWindow::EditExchangeList));
  Verify(connect(m_ui.addSymbol, &QAction::triggered, this,
                 &MainWindow::AddSymbol));

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

  Verify(connect(m_ui.showBalancesWindow, &QAction::triggered,
                 [this](bool isChecked) {
                   isChecked ? m_ui.balances->show() : m_ui.balances->hide();
                 }));
  Verify(connect(
      m_ui.showSecuritiesWindow, &QAction::triggered, [this](bool isChecked) {
        isChecked ? m_ui.securities->show() : m_ui.securities->hide();
      }));

  Verify(connect(m_ui.createNewChartWindows, &QAction::triggered, this,
                 &MainWindow::CreateNewChartWindows));
  Verify(connect(m_ui.createNewStrategyOperationsWindow, &QAction::triggered,
                 this, &MainWindow::CreateNewStrategyOperationsWindow));
  Verify(connect(m_ui.createNewStandaloneOrdersWindow, &QAction::triggered,
                 this, &MainWindow::CreateNewStandaloneOrderListWindow));
  Verify(connect(m_ui.createNewTotalResultsReportWindow, &QAction::triggered,
                 this, &MainWindow::CreateNewTotalResultsReportWindow));
}

FrontEnd::Engine &MainWindow::GetEngine() { return m_engine; }

const FrontEnd::Engine &MainWindow::GetEngine() const {
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
    actions = m_moduleDlls.back()
                  ->GetFunction<StrategyMenuActionList(FrontEnd::Engine &)>(
                      "CreateMenuActions")(m_engine);
  } catch (const std::exception &ex) {
    const auto &error =
        QString(tr("Failed to load module front-end: \"%1\".")).arg(ex.what());
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
  StrategyWidgetList widgets;
  try {
    widgets = factory(this);
  } catch (const std::exception &ex) {
    const auto &error =
        QString(R"(Failed to create new strategy window: "%1".)")
            .arg(ex.what());
    QMessageBox::critical(this, tr("Strategy error."), error,
                          QMessageBox::Abort);
    return;
  }
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
                       const QString &name, const ptr::ptree &config) {
    for (const auto &module : m_moduleDlls) {
      try {
        for (auto &widget :
             module->GetFunction<StrategyWidgetList(
                 Engine &, const QUuid &, const QUuid &, const QString &,
                 const ptr::ptree &, QWidget *)>("RestoreStrategyWidgets")(
                     m_engine, typeId, instanceId, name, config, this)) {
          widgets.emplace_back(widget.release());
        }
      } catch (const Dll::DllFuncException &) {
      }
    }
      });
  ShowModuleWindows(widgets);
}

void MainWindow::CreateNewChartWindows() {
  for (const auto &symbol :
       SymbolSelectionDialog(m_engine, this).RequestSymbols()) {
    CreateNewChartWindow(symbol);
  }
}

void MainWindow::CreateNewChartWindow(const QString &symbol) {
  class ChartMainWindow : public QMainWindow {
   public:
    explicit ChartMainWindow(const QString &symbol,
                             Engine &engine,
                             QWidget *parent)
        : QMainWindow(parent),
          m_symbol(symbol.toStdString()),
          m_engine(engine) {}
    ChartMainWindow(ChartMainWindow &&) = default;
    ChartMainWindow(const ChartMainWindow &) = delete;
    ChartMainWindow &operator=(ChartMainWindow &&) = delete;
    ChartMainWindow &operator=(const ChartMainWindow &) = delete;
    ~ChartMainWindow() {
      try {
        StopBars();
      } catch (...) {
        AssertFailNoException();
      }
    }
    void StartBars(const size_t numberOfFrames,
                   const size_t newNumberOfSecondsInFrame) {
      AssertEq(0, m_securities.size());
      m_period = pt::seconds(static_cast<long>(newNumberOfSecondsInFrame));
      const auto &startTime = m_engine.GetContext().GetCurrentTime() -
                              (m_period * static_cast<int>(numberOfFrames + 2));
      m_engine.GetContext().ForEachMarketDataSource(
          [this, startTime](MarketDataSource &source) {
            source.ForEachSecurity([this, startTime](Security &security) {
              if (security.GetSymbol().GetSymbol() != m_symbol) {
                return;
              }
              security.StartBars(startTime, m_period);
              Verify(m_securities.emplace(&security).second);
            });
          });
    }
    void StopBars() {
      if (m_period == pt::not_a_date_time) {
        return;
      }
      for (auto *security : m_securities) {
        security->StopBars(m_period);
      }
      m_securities.clear();
    }
    bool HasSecurity(const Security &security) const {
      return m_securities.count(const_cast<Security *>(&security)) != 0;
    }

   private:
    const std::string m_symbol;
    Engine &m_engine;
    boost::unordered_set<Security *> m_securities;
    pt::time_duration m_period;
  };

  auto window = boost::make_unique<ChartMainWindow>(symbol, GetEngine(), this);
  window->setWindowTitle(tr("%1 Candlestick Chart").arg(symbol));
  {
    auto &centralWidget = *new QWidget(&*window);
    auto &layout = *new QVBoxLayout(&centralWidget);
    centralWidget.setLayout(&layout);
    auto &toolbar = *new ChartToolbarWidget(&centralWidget);
    layout.addWidget(&toolbar);
    {
      const auto numberOfFrames = 100;
      auto &chart = *new CandlestickChartWidget(
          toolbar.GetNumberOfSecondsInFrame(), numberOfFrames, &centralWidget);
      auto &windowRef = *window;
      Verify(connect(
          &toolbar, &ChartToolbarWidget::NumberOfSecondsInFrameChange, &chart,
          [&windowRef, &chart,
           numberOfFrames](size_t newNumberOfSecondsInFrame) {
            windowRef.StopBars();
            windowRef.StartBars(numberOfFrames, newNumberOfSecondsInFrame);
            chart.SetNumberOfSecondsInFrame(newNumberOfSecondsInFrame);
          }));
      {
        const auto symbolStr = symbol.toStdString();
        Verify(connect(&m_engine, &Engine::BarUpdated, &chart,
                       [&windowRef, &chart, symbolStr](const Security *security,
                                                       const Bar &bar) {
                         if (!windowRef.HasSecurity(*security)) {
                           return;
                         }
                         chart.Update(bar);
                       }));
        window->StartBars(numberOfFrames, chart.GetNumberOfSecondsInFrame());
      }
      layout.addWidget(&chart);
    }
    window->setCentralWidget(&centralWidget);
  }
  window->setContentsMargins(0, 0, 0, 0);
  window->resize(600, 450);
  window->show();
  window->setAttribute(Qt::WA_DeleteOnClose);
  window.release();
}

void MainWindow::CreateNewOrderWindows(Security &security) {
  {
    const auto &it = m_orderWindows.find(&security);
    if (it != m_orderWindows.cend()) {
      it->second->activateWindow();
      it->second->raise();
      return;
    }
  }
  auto window = boost::make_unique<OrderWindow>(security, m_engine, this);
  Verify(connect(&*window, &OrderWindow::Closed,
                 [this, &security]() { m_orderWindows.erase(&security); }));
  window->show();
  m_orderWindows.emplace(&security, &*window);
  window->setAttribute(Qt::WA_DeleteOnClose);
  window.release();
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

void MainWindow::InitBalanceListWindow() {
  {
    auto &view = *new BalanceListView(this);
    {
      auto &model = *new SortFilterProxyModel(&view);
      model.setSourceModel(new BalanceListModel(m_engine, &view));
      view.setModel(&model);
    }
    m_ui.balances->setWidget(&view);
    m_ui.balances->setWindowTitle(view.windowTitle());
  }
  Verify(connect(m_ui.balances, &QDockWidget::visibilityChanged,
                 [this](bool isVisible) {
                   const QSignalBlocker blocker(m_ui.showBalancesWindow);
                   m_ui.showBalancesWindow->setChecked(isVisible);
                 }));
}

void MainWindow::InitSecurityListWindow() {
  {
    auto &view = *new SecurityListView(this);
    {
      auto &model = *new SortFilterProxyModel(&view);
      model.setSourceModel(new SecurityListModel(m_engine, &view));
      view.setModel(&model);
    }
    m_ui.securities->setWidget(&view);
    m_ui.securities->setWindowTitle(view.windowTitle());
    Verify(connect(&view, &SecurityListView::OrderRequested, this,
                   &MainWindow::CreateNewOrderWindows));
    Verify(connect(&view, &SecurityListView::ChartRequested, this,
                   &MainWindow::CreateNewChartWindow));
  }
  Verify(connect(m_ui.securities, &QDockWidget::visibilityChanged,
                 [this](bool isVisible) {
                   const QSignalBlocker blocker(m_ui.showSecuritiesWindow);
                   m_ui.showSecuritiesWindow->setChecked(isVisible);
                 }));
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

void MainWindow::EditExchangeList() {
  QDialog sourceListDialog(this);

  try {
    auto &listWidget = *new SourcesListWidget(
        m_engine.LoadConfig().get_child("sources", {}), &sourceListDialog);

    {
      sourceListDialog.setWindowTitle(QObject::tr("Source List"));
      auto &vLayout = *new QVBoxLayout(&sourceListDialog);
      sourceListDialog.setLayout(&vLayout);
      vLayout.addWidget(&listWidget);
      Verify(QObject::connect(
          &listWidget, &SourcesListWidget::SourceDoubleClicked,
          &sourceListDialog,
          [&listWidget, &sourceListDialog](const QModelIndex &index) {
            SourcePropertiesDialog dialog(listWidget.Dump(index),
                                          listWidget.GetImplementations(),
                                          &sourceListDialog);
            if (dialog.exec() != QDialog::Accepted) {
              return;
            }
            listWidget.AddSource(dialog.Dump());
          }));
      {
        auto &addButton =
            *new QPushButton(QObject::tr("Add New..."), &sourceListDialog);
        vLayout.addWidget(&addButton);
        Verify(QObject::connect(
            &addButton, &QPushButton::clicked, &sourceListDialog,
            [&listWidget, &sourceListDialog]() {
              SourcePropertiesDialog dialog(boost::none,
                                            listWidget.GetImplementations(),
                                            &sourceListDialog);
              if (dialog.exec() != QDialog::Accepted) {
                return;
              }
              listWidget.AddSource(dialog.Dump());
            }));
      }
      {
        auto &okButton =
            *new QPushButton(QObject::tr("Save"), &sourceListDialog);
        auto &cancelButton =
            *new QPushButton(QObject::tr("Cancel"), &sourceListDialog);
        Verify(QObject::connect(&okButton, &QPushButton::clicked,
                                &sourceListDialog, &QDialog::accept));
        Verify(QObject::connect(&cancelButton, &QPushButton::clicked,
                                &sourceListDialog, &QDialog::reject));
        auto &hLayout = *new QHBoxLayout(&sourceListDialog);
        hLayout.addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding,
                                        QSizePolicy::Minimum));
        hLayout.addWidget(&okButton);
        hLayout.addWidget(&cancelButton);
        vLayout.addLayout(&hLayout);
      }
    }

    if (sourceListDialog.exec() != QDialog::Accepted) {
      return;
    }

    {
      auto config = m_engine.LoadConfig();
      config.put_child("sources", listWidget.Dump());
      m_engine.StoreConfig(config);
    }

  } catch (const std::exception &ex) {
    const auto &error =
        QString(tr(R"(Failed to edit source list: "%1".)")).arg(ex.what());
    QMessageBox::critical(this, tr("Source list"), error, QMessageBox::Ignore);
    return;
  }

  QMessageBox::information(
      this, tr("Source list"),
      tr("Changes will take effect after the application restart."),
      QMessageBox::Ok);
}

void MainWindow::AddSymbol() {
  SymbolDialog dialog(this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  auto symbol = dialog.GetSymbol().toStdString();
  if (symbol.empty() || symbol[0] == '_') {
    return;
  }

  boost::to_upper(symbol);

  try {
    auto config = m_engine.LoadConfig();
    auto symbolsConfig = config.get_child("defaults.symbols");
    for (const auto &node : symbolsConfig) {
      if (node.second.get_value<std::string>() == symbol) {
        return;
      }
    }
    symbolsConfig.push_back({"", ptr::ptree().put("", symbol)});
    config.put_child("defaults.symbols", symbolsConfig);
    m_engine.StoreConfig(config);
  } catch (const std::exception &ex) {
    const auto &error =
        QString(tr(R"(Failed to edit source list: "%1".)")).arg(ex.what());
    QMessageBox::critical(this, tr("Source list"), error, QMessageBox::Ignore);
    return;
  }
  QMessageBox::information(
      this, tr("Symbol list"),
      tr("Changes will take effect after the application restart."),
      QMessageBox::Ok);
}
