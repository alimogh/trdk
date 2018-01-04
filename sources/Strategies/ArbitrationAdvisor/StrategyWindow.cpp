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
using namespace trdk::FrontEnd::Lib;
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
}

StrategyWindow::StrategyWindow(Engine &engine,
                               const boost::optional<QString> &defaultSymbol,
                               QWidget *parent)
    : Base(parent),
      m_engine(engine),
      m_tradingMode(TRADING_MODE_LIVE),
      m_instanceNumber(numberOfNextInstance++),
      m_strategy(nullptr),
      m_bestBuyTradingSystem(nullptr),
      m_bestSellTradingSystem(nullptr) {
  setAttribute(Qt::WA_DeleteOnClose);
  m_ui.setupUi(this);
  LoadSymbolList();
  if (defaultSymbol) {
    m_ui.symbol->setCurrentText(*defaultSymbol);
  } else {
    m_ui.symbol->setCurrentIndex(0);
  }
  m_symbolIndex = m_ui.symbol->currentIndex();
  InitBySelectedSymbol();
  ConnectSignals();
}

StrategyWindow::~StrategyWindow() {
  try {
    DeactivateAutoTrading();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
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
    closeEvent->accept();
    return;
  }
  const auto &response = QMessageBox::question(
      this, tr("Strategy closing"),
      tr("Are you sure you want to the close strategy window?\n\nIf strategy "
         "window will be closed - automatic trading will be stopped, all "
         "automatically opened position will not automatically closed.\n\n"
         "Continue and close strategy window for %1?")
          .arg(m_ui.symbol->currentText()),
      QMessageBox::Yes | QMessageBox::No);
  if (response == QMessageBox::Yes) {
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
  Verify(connect(m_ui.symbol, static_cast<void (QComboBox::*)(int)>(
                                  &QComboBox::currentIndexChanged),
                 this, &StrategyWindow::OnCurrentSymbolChange));

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
  Verify(connect(m_ui.maxQty, static_cast<void (QDoubleSpinBox::*)(double)>(
                                  &QDoubleSpinBox::valueChanged),
                 [this](double qty) {
                   UpdateAutoTradingLevel(m_ui.autoTradeLevel->value(), qty);
                 }));

  qRegisterMetaType<trdk::Strategies::ArbitrageAdvisor::Advice>(
      "trdk::Strategies::ArbitrageAdvisor::Advice");
  Verify(connect(this, &StrategyWindow::Advice, this,
                 &StrategyWindow::TakeAdvice, Qt::QueuedConnection));

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

void StrategyWindow::LoadSymbolList() {
  const IniFile conf(m_engine.GetConfigFilePath());
  const IniSectionRef defaults(conf, "Defaults");
  for (const std::string &symbol :
       defaults.ReadList("symbol_list", ",", true)) {
    m_ui.symbol->addItem(QString::fromStdString(symbol));
  }
}

void StrategyWindow::InitBySelectedSymbol() {
  const auto &symbol = m_ui.symbol->currentText().toStdString();

  static ids::random_generator generateStrategyId;
  const auto &strategyId = generateStrategyId();
  {
    const IniFile conf(m_engine.GetConfigFilePath());
    const IniSectionRef defaults(conf, "Defaults");
    std::ostringstream os;
    os << "[Strategy.Arbitrage/" << symbol << '/' << m_instanceNumber << "]"
       << std::endl
       << "module = ArbitrationAdvisor" << std::endl
       << "id = " << strategyId << std::endl
       << "is_enabled = true" << std::endl
       << "trading_mode = live" << std::endl
       << "title = " << symbol << " Arbitrage" << std::endl
       << "requires = Level 1 Updates[" << symbol << "]" << std::endl;
    const auto &copyKey = [&os, &conf](const char *key) {
      if (!conf.IsKeyExist("General", key)) {
        return;
      }
      os << key << " = " << conf.ReadKey("General", key) << std::endl;
    };
    copyKey("lowest_spread_percentage");
    copyKey("cross_arbitrage_mode");
    copyKey("stop_loss");
    m_engine.GetContext().Add(IniString(os.str()));
  }
  m_strategy = boost::polymorphic_downcast<aa::Strategy *>(
      &m_engine.GetContext().GetSrategy(strategyId));

  m_strategy->Invoke<aa::Strategy>([this](aa::Strategy &advisor) {
    advisor.SetupAdvising(m_ui.highlightLevel->value() / 100);
    Assert(!advisor.GetAutoTradingSettings() ? true : false);
  });

  m_strategy->Invoke<aa::Strategy>([this](aa::Strategy &advisor) {
    m_adviceConnection = advisor.SubscribeToAdvice(
        [this](const aa::Advice &advice) { emit Advice(advice); });
  });

  uint8_t biggestGetPricePrecision = 1;

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
        if (security.GetSymbol().GetSymbol() != symbol) {
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

    widgets->second->bid.SetPrecision(security.GetPricePrecision());
    widgets->second->ask.SetPrecision(security.GetPricePrecision());
    const Target target = {m_tradingMode, &security, &*widgets->second};
    Verify(m_targets.emplace(std::move(target)).second);
    if (biggestGetPricePrecision < security.GetPricePrecision()) {
      biggestGetPricePrecision = security.GetPricePrecision();
    }
  }

  m_bestSpreadAbsValue =
      PriceAdapter<QLabel>{*m_ui.bestSpreadAbsValue, biggestGetPricePrecision};

  setWindowTitle(m_ui.symbol->currentText() + " " + tr("Arbitrage") + " - " +
                 QCoreApplication::applicationName());

  for (const auto &target : m_targets) {
    target.widgets->Update(*target.security);
  }
}

void StrategyWindow::OnCurrentSymbolChange(int newSymbolIndex) {
  const QString &newSymbol = m_ui.symbol->itemText(newSymbolIndex);

  if (IsAutoTradingActivated()) {
    const QSignalBlocker blocker(*m_ui.symbol);
    m_ui.symbol->setCurrentIndex(m_symbolIndex);

    const auto &response = QMessageBox::question(
        this, tr("Strategy symbol change"),
        tr("Are you sure you want to change the symbol of strategy from %1 "
           "to "
           "%2?\n\nIf symbol will be changed - automatic trading will be "
           "stopped.\n\nClick Yes to continue and change symbol for this "
           "strategy  instance, choose No to start new strategy instance for "
           "%2, or click Cancel to cancel the action.")
            .arg(m_ui.symbol->currentText(), newSymbol),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (response != QMessageBox::Yes) {
      if (response == QMessageBox::No) {
        (new StrategyWindow(m_engine, newSymbol, parentWidget()))->show();
      }
      return;
    }
  }

  auto &newWindow = *(new StrategyWindow(m_engine, newSymbol, parentWidget()));
  newWindow.setGeometry(geometry());
  newWindow.adjustSize();
  m_ui.autoTrade->setChecked(false);
  close();
  newWindow.show();
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
    const auto &setSideSignal = [this, &advice](
        TargetSideWidget &sideWidget, bool isBest, Security &security,
        TradingSystem *&bestTradingSystem) {
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
  const auto &tragetIt = index.find(boost::make_tuple(
      tradingSystem, m_ui.symbol->currentText().toStdString()));
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
                             params, m_engine.GetOrderTradingSystemSlot(),
                             m_engine.GetRiskControl(m_tradingMode), side,
                             TIME_IN_FORCE_GTC, Milestones());
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to send order"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
    return;
  }
}

bool StrategyWindow::IsAutoTradingActivated() const {
  bool result = false;
  m_strategy->Invoke<aa::Strategy>(
      [this, &result](const aa::Strategy &advisor) {
        result = advisor.GetAutoTradingSettings() ? true : false;
      });
  AssertEq(result, m_ui.autoTrade->isChecked());
  return result;
}

void StrategyWindow::ToggleAutoTrading(bool activate) {
  try {
    m_strategy->Invoke<aa::Strategy>([this, activate](aa::Strategy &advisor) {
      AssertNe(activate, advisor.GetAutoTradingSettings() ? true : false);
      activate
          ? advisor.ActivateAutoTrading(
                {m_ui.autoTradeLevel->value() / 100, m_ui.maxQty->value()})
          : advisor.DeactivateAutoTrading();
    });
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to enable auto-trading"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  }
}

void StrategyWindow::DeactivateAutoTrading() {
  try {
    m_strategy->Invoke<aa::Strategy>(
        [](aa::Strategy &advisor) { advisor.DeactivateAutoTrading(); });
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to disable auto-trading"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  }
}

void StrategyWindow::UpdateAutoTradingLevel(const Double &level,
                                            const Qty &maxQty) {
  try {
    m_strategy->Invoke<aa::Strategy>(
        [this, &level, &maxQty](aa::Strategy &advisor) {
          if (!advisor.GetAutoTradingSettings()) {
            return;
          }
          advisor.ActivateAutoTrading({level / 100, maxQty});
        });
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to setup auto-trading"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  }
}

void StrategyWindow::UpdateAdviceLevel(double level) {
  try {
    m_strategy->Invoke<aa::Strategy>(
        [level](aa::Strategy &advisor) { advisor.SetupAdvising(level / 100); });
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to setup advice level"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  }
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<QWidget> CreateStrategyWidgets(Engine &engine,
                                               QWidget *parent) {
  return boost::make_unique<StrategyWindow>(engine, boost::none, parent);
}

////////////////////////////////////////////////////////////////////////////////
