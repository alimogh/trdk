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
#include "StrategyWindow.hpp"
#include "Advice.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::FrontEnd;
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace pt = boost::posix_time;
namespace ids = boost::uuids;
namespace aa = trdk::Strategies::ArbitrageAdvisor;

const std::string &StrategyWindow::Target::GetSymbol() const {
  return security->GetSymbol().GetSymbol();
}
const TradingSystem *StrategyWindow::Target::GetTradingSystem() const {
  return &security->GetContext().GetTradingSystem(
      security->GetSource().GetIndex(), tradingMode);
}

namespace {
size_t numberOfNextInstance = 1;
ids::random_generator generateStrategyId;
}  // namespace

StrategyWindow::StrategyWindow(Engine &engine,
                               const QString &symbol,
                               QWidget *parent)
    : Base(parent),
      m_engine(engine),
      m_tradingMode(TRADING_MODE_LIVE),
      m_symbol(symbol.toStdString()),
      m_strategy(GenerateNewStrategyInstance(generateStrategyId(),
                                             numberOfNextInstance++)),
      m_bestBuyTradingSystem(nullptr),
      m_bestSellTradingSystem(nullptr) {
  Init();
  StoreConfig(true);
}

StrategyWindow::StrategyWindow(Engine &engine,
                               const QUuid &strategyId,
                               const QString &config,
                               QWidget *parent)
    : Base(parent),
      m_engine(engine),
      m_tradingMode(TRADING_MODE_LIVE),
      m_symbol(ExtractSymbolFromConfig(config)),
      m_strategy(
          RestoreStrategyInstance(strategyId, numberOfNextInstance++, config)),
      m_bestBuyTradingSystem(nullptr),
      m_bestSellTradingSystem(nullptr) {
  Init();
}

StrategyWindow::~StrategyWindow() {
  try {
    if (!m_strategy.IsBlocked()) {
      DeactivateAutoTrading();
    }
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void StrategyWindow::Init() {
  setAttribute(Qt::WA_DeleteOnClose);

  m_ui.setupUi(this);
  m_ui.symbol->setText(QString::fromStdString(m_symbol));

  auto &context = m_engine.GetContext();
  for (size_t i = 0; i < context.GetNumberOfTradingSystems(); ++i) {
    auto &tradingSystem = context.GetTradingSystem(i, m_tradingMode);

    boost::optional<Security *> securityPtr;
    for (size_t sourceI = 0;
         sourceI < context.GetNumberOfMarketDataSources() && !securityPtr;
         ++sourceI) {
      auto &source = context.GetMarketDataSource(sourceI);
      if (source.GetInstanceName() != tradingSystem.GetInstanceName()) {
        continue;
      }
      securityPtr = nullptr;
      source.ForEachSecurity([&](Security &security) {
        if (security.GetSymbol().GetSymbol() != m_symbol) {
          return;
        }
        Assert(securityPtr);
        Assert(!*securityPtr);
        securityPtr = &security;
      });
      break;
    }
    if (!securityPtr) {
      QMessageBox::warning(
          this, tr("Configuration warning"),
          tr("Trading system \"%1\" does not have market data source.")
              .arg(QString::fromStdString(tradingSystem.GetInstanceName())),
          QMessageBox::Ignore);
      continue;
    } else if (!*securityPtr) {
      // This symbol is not supported by this market data source.
      continue;
    }
    auto &security = **securityPtr;

    auto widgets = m_targetWidgets.find(&tradingSystem);
    if (widgets == m_targetWidgets.cend()) {
      widgets =
          m_targetWidgets
              .emplace(
                  &tradingSystem,
                  boost::make_unique<TargetWidgets>(
                      QString::fromStdString(tradingSystem.GetInstanceName()),
                      this))
              .first;
      AddTargetWidgets(*widgets->second);
    }

    const Target target = {m_tradingMode, &security, &*widgets->second};
    Verify(m_targets.emplace(std::move(target)).second);
  }

  m_bestSpreadAbsValue = PriceAdapter<QLabel>{*m_ui.bestSpreadAbsValue};

  setWindowTitle(m_ui.symbol->text() + " " + tr("Arbitrage") + " - " +
                 QCoreApplication::applicationName());

  for (const auto &target : m_targets) {
    target.widgets->Update(*target.security);
  }

  m_ui.highlightLevel->setValue(
      m_strategy.GetMinPriceDifferenceRatioToAdvice() * 100);
  m_ui.autoTrade->setChecked(m_strategy.GetTradingSettings().isEnabled);
  m_ui.autoTradeLevel->setValue(
      m_strategy.GetTradingSettings().minPriceDifferenceRatio * 100);
  m_ui.maxQty->setValue(m_strategy.GetTradingSettings().maxQty);

  ConnectSignals();
}

void StrategyWindow::AddTargetWidgets(TargetWidgets &widgets) {
  const auto row = static_cast<int>(m_targetWidgets.size());
  int column = 0;
  m_ui.targets->addWidget(&widgets.title, row, column++);
  m_ui.targets->addWidget(&widgets.bid, row, column++);
  m_ui.targets->addWidget(&widgets.ask, row, column++);
  m_ui.targets->addWidget(&widgets.actions, row, column++);
}

void StrategyWindow::closeEvent(QCloseEvent *closeEvent) {
  if (!IsAutoTradingActivated()) {
    StoreConfig(false);
    closeEvent->accept();
    return;
  }
  const auto &response = QMessageBox::question(
      this, tr("Strategy closing"),
      tr("Are you sure you want to the close strategy window?\n\nIf strategy "
         "window will be closed - automatic trading will be stopped, all "
         "automatically opened position will not automatically closed.\n\n"
         "Continue and close strategy window for %1?")
          .arg(m_ui.symbol->text()),
      QMessageBox::Yes | QMessageBox::No);
  if (response == QMessageBox::Yes) {
    StoreConfig(false);
    closeEvent->accept();
  } else {
    closeEvent->ignore();
  }
}

QSize StrategyWindow::sizeHint() const {
  auto result = Base::sizeHint();
  result.setWidth(static_cast<int>(result.width() * 0.9));
  result.setHeight(static_cast<int>(result.height() * 0.7));
  return result;
}

void StrategyWindow::ConnectSignals() {
  m_adviceConnection = m_strategy.SubscribeToAdvice(
      [this](const aa::Advice &advice) { emit Advice(advice); });
  m_tradingSignalCheckErrorsConnection =
      m_strategy.SubscribeToTradingSignalCheckErrors(
          [this](const std::vector<std::string> &errors) {
            emit SignalCheckErrors(errors);
          });
  m_blockConnection =
      m_strategy.SubscribeToBlocking([this](const std::string *reasonSource) {
        QString reason;
        if (reasonSource) {
          reason = QString::fromStdString(*reasonSource);
        }
        emit Blocked(reason);
      });

  Verify(connect(m_ui.highlightLevel,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 this, &StrategyWindow::UpdateAdviceLevel));

  Verify(connect(m_ui.autoTrade, &QCheckBox::toggled, this,
                 &StrategyWindow::ToggleAutoTrading));
  Verify(connect(m_ui.autoTradeLevel,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double level) {
                   UpdateAutoTradingLevel(level, m_ui.maxQty->value());
                 }));
  Verify(connect(m_ui.maxQty,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double qty) {
                   UpdateAutoTradingLevel(m_ui.autoTradeLevel->value(), qty);
                 }));

  qRegisterMetaType<trdk::Strategies::ArbitrageAdvisor::Advice>(
      "trdk::Strategies::ArbitrageAdvisor::Advice");
  Verify(connect(this, &StrategyWindow::Advice, this,
                 &StrategyWindow::TakeAdvice, Qt::QueuedConnection));

  qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");
  Verify(connect(this, &StrategyWindow::SignalCheckErrors, this,
                 &StrategyWindow::OnSignalCheckErrors, Qt::QueuedConnection));

  Verify(connect(this, &StrategyWindow::Blocked, this,
                 &StrategyWindow::OnBlocked, Qt::QueuedConnection));

  for (auto &target : m_targetWidgets) {
    auto *const tradingSystem = target.first;
    Verify(connect(&target.second->actions, &TargetActionsWidget::Order,
                   [this, tradingSystem](const OrderSide &side) {
                     SendOrder(side, tradingSystem);
                   }));
  }

  Verify(connect(m_ui.bestSell, &QPushButton::clicked,
                 [this]() { SendOrder(ORDER_SIDE_SELL, nullptr); }));
  Verify(connect(m_ui.bestBuy, &QPushButton::clicked,
                 [this]() { SendOrder(ORDER_SIDE_BUY, nullptr); }));
}

aa::Strategy &StrategyWindow::GenerateNewStrategyInstance(
    const ids::uuid &strategyId, size_t instanceNumber) {
  bool isLowestSpreadEnabed = false;
  Double lowestSpreadPercentage = 0;
  bool isStopLossEnabled = false;
  size_t stopLossDelaySec = 0;
  {
    const IniFile conf(m_engine.GetConfigFilePath());
    const IniSectionRef general(conf, "General");
    {
      const std::string key = "lowest_spread_enabled";
      if (general.IsKeyExist(key)) {
        isLowestSpreadEnabed = general.ReadBoolKey(key);
      }
    }
    {
      const std::string key = "lowest_spread_percentage";
      if (general.IsKeyExist(key)) {
        lowestSpreadPercentage = general.ReadTypedKey<Double>(key);
      }
    }
    {
      const std::string key = "stop_loss_enabled";
      if (general.IsKeyExist(key)) {
        isStopLossEnabled = general.ReadBoolKey(key);
      }
    }
    {
      const std::string key = "stop_loss_delay_sec";
      if (general.IsKeyExist(key)) {
        stopLossDelaySec = general.ReadTypedKey<size_t>(key);
      }
    }
  }
  return CreateStrategyInstance(
      strategyId, instanceNumber,
      CreateConfig(strategyId, .6, .6, 100000000, isLowestSpreadEnabed,
                   lowestSpreadPercentage, isStopLossEnabled,
                   stopLossDelaySec));
}

aa::Strategy &StrategyWindow::RestoreStrategyInstance(const QUuid &strategyId,
                                                      size_t instanceNumber,
                                                      const QString &config) {
  return CreateStrategyInstance(ConvertToBoostUuid(strategyId), instanceNumber,
                                config.toStdString());
}

aa::Strategy &StrategyWindow::CreateStrategyInstance(
    const ids::uuid &strategyId,
    size_t instanceNumber,
    const std::string &config) {
  std::ostringstream ini;
  ini << "[Strategy.Arbitrage/" << m_symbol << '/' << instanceNumber << "]"
      << std::endl;
  ini << config;
  m_engine.GetContext().Add(IniString(ini.str()));
  return *boost::polymorphic_downcast<aa::Strategy *>(
      &m_engine.GetContext().GetSrategy(strategyId));
}

void StrategyWindow::TakeAdvice(const aa::Advice &advice) {
  Assert(advice.security);

  {
    const auto &targetIndex = m_targets.get<BySecurity>();
    const auto updateTargeIt = targetIndex.find(advice.security);
    if (updateTargeIt == targetIndex.cend()) {
      return;
    }
    updateTargeIt->widgets->Update(advice);
  }

  {
    m_bestSpreadAbsValue.Set(advice.bestSpreadValue);
    if (advice.bestSpreadRatio.IsNotNan()) {
      m_ui.bestSpreadPencentsValue->setText(
          QString::number(advice.bestSpreadRatio * 100, 'f', 2) + "%");
      if (advice.isSignaled) {
        static const QString style("background-color: rgb(0, 146, 68);");
        m_ui.spread->setStyleSheet(style);
      } else if (advice.bestSpreadRatio > 0) {
        static const QString style("color: rgb(0, 195, 91);");
        m_ui.spread->setStyleSheet(style);
      } else {
        m_ui.spread->setStyleSheet(styleSheet());
      }
    } else {
      static const QString text("-.--%");
      m_ui.bestSpreadPencentsValue->setText(text);
      m_ui.spread->setStyleSheet(styleSheet());
    }
  }

  {
    const auto &setSideSignal =
        [this, &advice](TargetSideWidget &sideWidget, bool isBest,
                        Security &security, TradingSystem *&bestTradingSystem) {
          sideWidget.Highlight(isBest, advice.isSignaled);
          if (isBest) {
            bestTradingSystem = &security.GetContext().GetTradingSystem(
                security.GetSource().GetIndex(), m_tradingMode);
          }
        };
    const auto &targetIndex = m_targets.get<BySecurity>();
    for (const auto &signal : advice.securitySignals) {
      const auto signalTargetIt = targetIndex.find(signal.security);
      Assert(signalTargetIt != targetIndex.cend());
      if (signalTargetIt == targetIndex.cend()) {
        continue;
      }
      setSideSignal(signalTargetIt->widgets->bid, signal.isBestBid,
                    *signal.security, m_bestSellTradingSystem);
      setSideSignal(signalTargetIt->widgets->ask, signal.isBestAsk,
                    *signal.security, m_bestBuyTradingSystem);
    }
  }
}

void StrategyWindow::OnSignalCheckErrors(
    const std::vector<std::string> &errors) {
  m_ui.events->clear();
  for (const auto &error : errors) {
    m_ui.events->appendPlainText(QString::fromStdString(error));
  }
}

void StrategyWindow::OnBlocked(const QString &reason) {
  {
    const QSignalBlocker blocker(*m_ui.autoTrade);
    m_ui.autoTrade->setChecked(false);
  }
  {
    m_ui.symbol->setEnabled(false);
    m_ui.highlightLevel->setEnabled(false);
    m_ui.autoTrade->setEnabled(false);
    m_ui.autoTradeLevel->setEnabled(false);
    m_ui.maxQty->setEnabled(false);
  }
  ShowBlockedStrategyMessage(reason, this);
}

void StrategyWindow::SendOrder(const OrderSide &side,
                               TradingSystem *tradingSystem) {
  if (!tradingSystem) {
    tradingSystem = side == ORDER_SIDE_BUY ? m_bestBuyTradingSystem
                                           : m_bestSellTradingSystem;
    if (!tradingSystem) {
      QMessageBox::warning(
          this, tr("Failed to send order"),
          QString(
              "There is no suitable exchange to %1 at best price. Wait until "
              "price changed or send a manual order.")
              .arg(side == ORDER_SIDE_BUY ? tr("buy") : tr("sell")),
          QMessageBox::Ok);
      return;
    }
  }
  Assert(tradingSystem);
  const auto &index = m_targets.get<BySymbol>();
  const auto &tragetIt = index.find(
      boost::make_tuple(tradingSystem, m_ui.symbol->text().toStdString()));
  Assert(tragetIt != index.cend());
  if (tragetIt == index.cend()) {
    return;
  }
  Assert(tragetIt->security);
  Security &security = *tragetIt->security;

  static const OrderParams params;
  try {
    tradingSystem->SendOrder(security, security.GetSymbol().GetCurrency(),
                             m_ui.maxQty->value(),
                             side == ORDER_SIDE_BUY ? security.GetAskPrice()
                                                    : security.GetBidPrice(),
                             params, boost::make_unique<OrderStatusNotifier>(),
                             m_engine.GetRiskControl(m_tradingMode), side,
                             TIME_IN_FORCE_GTC, Milestones());
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to send order"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
    return;
  }
}

bool StrategyWindow::IsAutoTradingActivated() const {
  return !m_strategy.IsBlocked() && m_strategy.GetTradingSettings().isEnabled;
}

void StrategyWindow::ToggleAutoTrading(bool activate) {
  try {
    m_strategy.SetTradingSettings(
        {activate, m_ui.autoTradeLevel->value() / 100, m_ui.maxQty->value()});
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to enable auto-trading"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  }
  StoreConfig(true);
}

void StrategyWindow::DeactivateAutoTrading() {
  try {
    m_strategy.SetTradingSettings(
        {false, m_ui.autoTradeLevel->value() / 100, m_ui.maxQty->value()});
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to disable auto-trading"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  }
}

void StrategyWindow::UpdateAutoTradingLevel(const Double &level,
                                            const Qty &maxQty) {
  try {
    m_strategy.SetTradingSettings(
        {m_ui.autoTrade->isChecked(), level / 100, maxQty});
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to setup auto-trading"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  }
  StoreConfig(true);
}

void StrategyWindow::UpdateAdviceLevel(double level) {
  try {
    m_strategy.SetMinPriceDifferenceRatioToAdvice(level / 100);
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to setup advice level"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  }
  StoreConfig(true);
}

std::string StrategyWindow::CreateConfig(
    const ids::uuid &strategyId,
    const Double &minPriceDifferenceToHighlightPercentage,
    const Double &minPriceDifferenceToTradePercentage,
    const Qty &maxQty,
    bool isLowestSpreadEnabled,
    const Double &lowestSpreadPercentage,
    bool isStopLossEnabled,
    size_t stopLossDelaySec) const {
  std::ostringstream result;
  result << "module = ArbitrationAdvisor" << std::endl;
  result << "id = " << strategyId << std::endl;
  result << "is_enabled = true" << std::endl;
  result << "trading_mode = live" << std::endl;
  result << "title = Arbitrage" << std::endl;
  result << "requires = Level 1 Updates[" << m_symbol << "]" << std::endl;
  result << "symbol = " << m_symbol << std::endl;
  result << "min_price_difference_to_highlight_percentage = "
         << minPriceDifferenceToHighlightPercentage << std::endl;
  result << "min_price_difference_to_trade_percentage = "
         << minPriceDifferenceToTradePercentage << std::endl;
  result << "max_qty = " << maxQty << std::endl;
  result << "lowest_spread_enabled = "
         << (isLowestSpreadEnabled ? "true" : "false") << std::endl;
  result << "lowest_spread_percentage = " << lowestSpreadPercentage
         << std::endl;
  result << "stop_loss_enabled = " << (isStopLossEnabled ? "true" : "false")
         << std::endl;
  result << "stop_loss_delay_sec = " << stopLossDelaySec << std::endl;
  return result.str();
}

std::string StrategyWindow::DumpConfig() const {
  const auto &tradingSettings = m_strategy.GetTradingSettings();
  return CreateConfig(
      m_strategy.GetId(), m_strategy.GetMinPriceDifferenceRatioToAdvice() * 100,
      tradingSettings.minPriceDifferenceRatio * 100, tradingSettings.maxQty,
      m_strategy.IsLowestSpreadEnabled(),
      m_strategy.GetLowestSpreadRatio() * 100, m_strategy.IsStopLossEnabled(),
      m_strategy.GetStopLossDelay().total_seconds());
}

void StrategyWindow::StoreConfig(bool isActive) {
  m_engine.StoreConfig(m_strategy, QString::fromStdString(DumpConfig()),
                       isActive);
}

////////////////////////////////////////////////////////////////////////////////

StrategyMenuActionList CreateMenuActions(Engine &engine) {
  return {1,
          {QObject::tr("&Arbitration Advisor..."),
           [&engine](QWidget *parent) -> StrategyWidgetList {
             StrategyWidgetList result;
             for (const auto &symbol :
                  SymbolSelectionDialog(engine, parent).RequestSymbols()) {
               result.emplace_back(
                   boost::make_unique<StrategyWindow>(engine, symbol, parent));
             }
             return result;
           }}};
}

StrategyWidgetList RestoreStrategyWidgets(Engine &engine,
                                          const QUuid &typeId,
                                          const QUuid &instanceId,
                                          const QString &config,
                                          QWidget *parent) {
  StrategyWidgetList result;
  if (ConvertToBoostUuid(typeId) == aa::Strategy::typeId) {
    result.emplace_back(
        boost::make_unique<StrategyWindow>(engine, instanceId, config, parent));
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
