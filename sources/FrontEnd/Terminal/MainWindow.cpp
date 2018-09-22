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
#include "Lib/DropCopy.hpp"
#include "Lib/Engine.hpp"
#include "Lib/MarketScannerModel.hpp"
#include "Lib/MarketScannerView.hpp"
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
#include "Lib/WalletDepositDialog.hpp"
#include "Lib/WalletSettingsDialog.hpp"

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
  InitMarketScannerWindow();

  CreateNewStrategyOperationsWindow();
  CreateNewStandaloneOrderListWindow();
  CreateNewTotalResultsReportWindow();

  m_ui.centalTabs->setCurrentIndex(0);

  ConnectSignals();

  ReloadWalletsConfig();
}

MainWindow::~MainWindow() = default;

void MainWindow::ConnectSignals() {
  Verify(connect(m_ui.editExchangeList, &QAction::triggered, this,
                 &MainWindow::EditExchangeList));
  Verify(connect(m_ui.addDefaultSymbol, &QAction::triggered, this,
                 &MainWindow::AddDefaultSymbol));

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

  Verify(connect(m_ui.showBalancesWindow, &QAction::triggered,
                 [this](const bool isChecked) {
                   isChecked ? m_ui.balances->show() : m_ui.balances->hide();
                 }));
  Verify(connect(m_ui.showSecuritiesWindow, &QAction::triggered,
                 [this](const bool isChecked) {
                   isChecked ? m_ui.securities->show()
                             : m_ui.securities->hide();
                 }));
  Verify(connect(m_ui.showMarketScannerWindow, &QAction::triggered,
                 [this](const bool isChecked) {
                   isChecked ? m_ui.marketScanner->show()
                             : m_ui.marketScanner->hide();
                 }));

  Verify(connect(m_ui.createNewChartWindows, &QAction::triggered, this,
                 &MainWindow::CreateNewChartWindows));
  Verify(connect(m_ui.createNewStrategyOperationsWindow, &QAction::triggered,
                 this, &MainWindow::CreateNewStrategyOperationsWindow));
  Verify(connect(m_ui.createNewStandaloneOrdersWindow, &QAction::triggered,
                 this, &MainWindow::CreateNewStandaloneOrderListWindow));
  Verify(connect(m_ui.createNewTotalResultsReportWindow, &QAction::triggered,
                 this, &MainWindow::CreateNewTotalResultsReportWindow));

  Verify(connect(&m_engine.GetDropCopy(), &FrontEnd::DropCopy::BalanceUpdate,
                 this, &MainWindow::RechargeWallet, Qt::QueuedConnection));
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
            continue;
          }
        }
      });
  ShowModuleWindows(widgets);

  boost::polymorphic_downcast<MarketScannerModel *>(
      boost::polymorphic_downcast<QAbstractProxyModel *>(
          boost::polymorphic_downcast<QTableView *>(
              m_ui.marketScanner->widget())
              ->model())
          ->sourceModel())
      ->Refresh();
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
    ChartMainWindow(ChartMainWindow &&) = delete;
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
           numberOfFrames](const size_t newNumberOfSecondsInFrame) {
            windowRef.StopBars();
            windowRef.StartBars(numberOfFrames, newNumberOfSecondsInFrame);
            chart.SetNumberOfSecondsInFrame(newNumberOfSecondsInFrame);
          }));
      {
        const auto symbolStr = symbol.toStdString();
        Verify(connect(&m_engine, &Engine::BarUpdate, &chart,
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
    Verify(connect(&view, &BalanceListView::WalletSettingsRequested, this,
                   &MainWindow::ShowWalletSettings));
    Verify(connect(&view, &BalanceListView::WalletDepositTransactionRequest,
                   this, &MainWindow::ShowWalletDepositDialog));
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
                 [this](const bool isVisible) {
                   const QSignalBlocker blocker(m_ui.showSecuritiesWindow);
                   m_ui.showSecuritiesWindow->setChecked(isVisible);
                 }));
}

void MainWindow::InitMarketScannerWindow() {
  {
    auto &view = *new MarketScannerView(this);
    {
      auto &model = *new SortFilterProxyModel(&view);
      model.setSourceModel(new MarketScannerModel(m_engine, &view));
      view.setModel(&model);
    }
    m_ui.marketScanner->setWidget(&view);
    m_ui.marketScanner->setWindowTitle(view.windowTitle());
    Verify(connect(&view, &MarketScannerView::StrategyRequested, this,
                   &MainWindow::ShowRequestedStrategy));
  }
  Verify(connect(m_ui.marketScanner, &QDockWidget::visibilityChanged,
                 [this](const bool isVisible) {
                   const QSignalBlocker blocker(m_ui.showSecuritiesWindow);
                   m_ui.showMarketScannerWindow->setChecked(isVisible);
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

void MainWindow::AddDefaultSymbol() {
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

void MainWindow::ShowRequestedStrategy(const QString &title,
                                       const QString &module,
                                       const QString &factoryName,
                                       const QString &params) {
  for (const auto &it : children()) {
    const auto widget = dynamic_cast<QWidget *>(it);
    if (!widget) {
      continue;
    }
    if (widget->windowTitle() == title) {
      widget->activateWindow();
      widget->raise();
      return;
    }
  }

  for (auto &dll : m_moduleDlls) {
    if (dll->GetOriginalFile().filename().replace_extension("") !=
        module.toStdString()) {
      continue;
    }
    boost::function<StrategyWidgetList(FrontEnd::Engine &, const QString &,
                                       const QWidget *parent)>
        factory;
    try {
      factory = dll->GetFunction<StrategyWidgetList(
          FrontEnd::Engine &, const QString &, const QWidget *parent)>(
          factoryName.toStdString());
    } catch (const std::exception &ex) {
      const auto &error =
          QString(tr("Failed to load module front-end: \"%1\"."))
              .arg(ex.what());
      QMessageBox::critical(this, tr("Failed to load module."), error,
                            QMessageBox::Ignore);
      return;
    }
    StrategyWidgetList widgets;
    try {
      widgets = factory(m_engine, params, this);
    } catch (const std::exception &ex) {
      const auto &error =
          QString(R"(Failed to create new strategy window: "%1".)")
              .arg(ex.what());
      QMessageBox::critical(this, tr("Strategy error."), error,
                            QMessageBox::Abort);
      return;
    }
    ShowModuleWindows(widgets);
    return;
  }
  {
    const auto &error =
        QString(tr("Failed to find module \"%1\".")).arg(module);
    QMessageBox::critical(this, tr("Failed to load module."), error,
                          QMessageBox::Ignore);
  }
}

void MainWindow::ShowWalletSettings(const QString symbol,
                                    const TradingSystem &tradingSystem) {
  WalletSettingsDialog dialog(m_engine, symbol, tradingSystem, m_walletsConfig,
                              false, this);
  if (dialog.exec() == QDialog::Accepted) {
    StoreWalletsConfig(dialog.GetConfig().Dump(), symbol, tradingSystem);
  }
}

void MainWindow::ShowWalletDepositDialog(QString symbol,
                                         const TradingSystem &tradingSystem) {
  for (;;) {
    if (!m_walletsConfig.GetAddress(symbol, tradingSystem).isEmpty()) {
      break;
    }
    if (QMessageBox::information(this, tr("Address is not set"),
                                 tr("Wallet address is not set yet. Please "
                                    "provide wallet address to deposit funds."),
                                 QMessageBox::Ok | QMessageBox::Cancel) !=
        QMessageBox::Ok) {
      return;
    }
    WalletSettingsDialog settingsDialog(m_engine, symbol, tradingSystem,
                                        m_walletsConfig, true, this);
    if (settingsDialog.exec() != QDialog::Accepted) {
      return;
    }
    StoreWalletsConfig(settingsDialog.GetConfig().Dump(), symbol,
                       tradingSystem);
  }

  WalletDepositDialog dialog(m_engine, std::move(symbol), tradingSystem,
                             m_walletsConfig, this);
  if (dialog.exec() != WalletDepositDialog::Accepted) {
    return;
  }

  try {
    dialog.GetSource().Withdraw(dialog.GetSymbol().toStdString(),
                                dialog.GetVolume(),
                                dialog.GetTargetAddress().toStdString());
  } catch (const Exception &ex) {
    const auto &error =
        QString(tr("Failed to deposit funds: \"%1\".")).arg(ex.what());
    QMessageBox::critical(this, tr("Failed to deposit funds."), error,
                          QMessageBox::Ok);
  }
}

void MainWindow::RechargeWallet(const TradingSystem *tradingSystem,
                                const std::string &symbol,
                                const Volume &available,
                                const Volume &locked) {
  const auto &symbolIt =
      m_walletsConfig.Get().find(QString::fromStdString(symbol));
  if (symbolIt == m_walletsConfig.Get().cend()) {
    return;
  }
  const auto &tradingSystemIt = symbolIt.value().find(tradingSystem);
  if (tradingSystemIt == symbolIt.value().cend()) {
    return;
  }
  const auto &recharging = tradingSystemIt->second.wallet.recharging;
  if (!recharging) {
    return;
  }
  Assert(!tradingSystemIt->second.wallet.address->isEmpty());
  auto transactionVolume =
      recharging->minDepositToRecharge - available + locked;
  if (transactionVolume < 0) {
    return;
  }
  if (transactionVolume < recharging->minRechargingTransactionVolume) {
    transactionVolume = recharging->minRechargingTransactionVolume;
  }
  const auto &sourceIt = symbolIt->find(recharging->source);
  Assert(sourceIt != symbolIt->cend());
  if (sourceIt == symbolIt->cend()) {
    return;
  }
  Assert(sourceIt->second.source);
  if (!sourceIt->second.source) {
    return;
  }
  const auto availableInSource =
      recharging->source->GetBalances().GetAvailableToTrade(symbol) -
      sourceIt->second.source->minDeposit;
  if (availableInSource <= 0) {
    return;
  }
  if (availableInSource < transactionVolume) {
    transactionVolume = availableInSource;
  }
  auto &source = *recharging->source;
  const auto address = *tradingSystemIt->second.wallet.address;
  m_engine.GetContext().GetTimer().Schedule(
      [&source, address, symbol, transactionVolume]() {
        source.Withdraw(symbol, transactionVolume, address.toStdString());
      },
      m_timerScope);
}

void MainWindow::ReloadWalletsConfig() {
  m_walletsConfig =
      WalletsConfig{m_engine, m_engine.LoadConfig().get_child("wallets", {})};
}

void MainWindow::StoreWalletsConfig(const ptr::ptree &config,
                                    const QString &symbolQStr,
                                    const TradingSystem &tradingSystem) {
  auto fullConfig = m_engine.LoadConfig();
  fullConfig.put_child("wallets", config);
  m_engine.StoreConfig(fullConfig);

  ReloadWalletsConfig();

  const auto &symbol = symbolQStr.toStdString();
  Volume available = 0;
  Volume locked = 0;
  tradingSystem.GetBalances().ForEach(
      [&symbol, &available, &locked](const std::string &balanceSymbol,
                                     const Volume &balanceAvailable,
                                     const Volume &balanceLocked) {
        if (symbol == balanceSymbol) {
          available = balanceAvailable;
          locked = balanceLocked;
        }
      });
  RechargeWallet(&tradingSystem, symbol, available, locked);
}
