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
      m_tradingMode(TRADING_MODE_LIVE),
      m_instanceNumber(numberOfNextInstance++),
      m_engine(engine),
      m_symbol(-1),
      m_instanceData({}) {
  setAttribute(Qt::WA_DeleteOnClose);

  m_ui.setupUi(this);

  AddTarget("Bittrex", "bittrex", &InstanceData::bittrexTradingSystem);
  //   AddTarget("Novaexchange", "novaexchange",
  //             &InstanceData::novaexchangeTradingSystem);
  AddTarget("Yobit.net", "yobitnet", &InstanceData::yobitnetTradingSystem);
  AddTarget("C-CEX", "ccex", &InstanceData::ccexTradingSystem);
  // AddTarget("GDAX", "gdax", &InstanceData::gdaxTradingSystem);

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
                 this, &StrategyWindow::UpdateAutoTradingLevel));

  qRegisterMetaType<trdk::Strategies::ArbitrageAdvisor::Advice>(
      "trdk::Strategies::ArbitrageAdvisor::Advice");
  Verify(connect(this, &StrategyWindow::Advice, this,
                 &StrategyWindow::TakeAdvice, Qt::QueuedConnection));

  LoadSymbols(defaultSymbol);

  adjustSize();
}

StrategyWindow::~StrategyWindow() {
  try {
    DeactivateAutoTrading();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void StrategyWindow::AddTarget(
    const QString &name,
    const std::string &targetId,
    TradingSystem *InstanceData::*tradingSystemField) {
  const auto &it = m_targetWidgets.emplace(
      boost::to_lower_copy(targetId),
      boost::make_unique<TargetWidgets>(tradingSystemField, this));
  Assert(it.second);
  if (!it.second) {
    throw LogicError(
        "Internal logic error: Failed to add the same target twice");
  }
  auto &widgets = *it.first->second;

  widgets.title.SetTitle(name);

  {
    const auto row = static_cast<int>(m_targetWidgets.size());
    int column = 0;
    m_ui.targets->addWidget(&widgets.title, row, column++);
    m_ui.targets->addWidget(&widgets.bid, row, column++);
    m_ui.targets->addWidget(&widgets.ask, row, column++);
    m_ui.targets->addWidget(&widgets.actions, row, column++);
  }

  Verify(connect(&widgets.actions, &TargetActionsWidget::Order,
                 [this, tradingSystemField](const OrderSide &side) {
                   TradingSystem *const tradingSystem =
                       m_instanceData.*tradingSystemField;
                   Assert(tradingSystem);
                   SendOrder(side, tradingSystem);
                 }));
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
         "automatically opened position will not automatically "
         "closed.\n\nContinue and close strategy window for %1?")
          .arg(m_ui.symbol->itemText(m_symbol)),
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

void StrategyWindow::LoadSymbols(
    const boost::optional<QString> &defaultSymbol) {
  {
    IniFile conf(m_engine.GetConfigFilePath());
    IniSectionRef defaults(conf, "Defaults");
    const SignalsScopedBlocker blocker(*m_ui.symbol);
    for (const std::string &symbol :
         defaults.ReadList("symbol_list", ",", true)) {
      m_ui.symbol->addItem(QString::fromStdString(symbol));
    }
  }
  if (defaultSymbol) {
    m_ui.symbol->setCurrentText(*defaultSymbol);
  } else {
    SetCurrentSymbol(m_ui.symbol->currentIndex());
  }
}

void StrategyWindow::OnCurrentSymbolChange(int newSymbolIndex) {
  if (m_symbol >= 0) {
    if (m_symbol == newSymbolIndex) {
      return;
    }

    if (IsAutoTradingActivated()) {
      const QString &symbol = m_ui.symbol->itemText(newSymbolIndex);

      const SignalsScopedBlocker blocker(*m_ui.symbol);
      m_ui.symbol->setCurrentIndex(m_symbol);

      const auto &response = QMessageBox::question(
          this, tr("Strategy symbol change"),
          tr("Are you sure you want to change the symbol of strategy from %1 "
             "to %2?\n\nIf symbol will be changed - automatic trading will be"
             " stopped.\n\nClick Yes to continue and change symbol for this"
             " strategy  instance, choose No to start new strategy instance"
             " for %2, or click Cancel to cancel the action.")
              .arg(m_ui.symbol->itemText(m_symbol), symbol),
          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
      if (response != QMessageBox::Yes) {
        if (response == QMessageBox::No) {
          (new StrategyWindow(m_engine, symbol, parentWidget()))->show();
        }
        return;
      }

      m_ui.symbol->setCurrentIndex(newSymbolIndex);
    }
  }
  SetCurrentSymbol(newSymbolIndex);
}

void StrategyWindow::SetCurrentSymbol(int symbolIndex) {
  DeactivateAutoTrading();

  const auto &symbol = m_ui.symbol->itemText(symbolIndex).toStdString();

  auto strategyId = m_strategiesUuids.find(symbol);
  if (strategyId == m_strategiesUuids.cend()) {
    static ids::random_generator generateId;
    strategyId = m_strategiesUuids.emplace(symbol, generateId()).first;
    std::ostringstream conf;
    conf << "[Strategy.Arbitrage" << symbol << m_instanceNumber << "]"
         << std::endl
         << "module = ArbitrationAdvisor" << std::endl
         << "id = " << strategyId->second << std::endl
         << "is_enabled = true" << std::endl
         << "trading_mode = live" << std::endl
         << "title = " << symbol << " Arbitrage" << std::endl
         << "requires = Level 1 Updates[" << symbol << "]" << std::endl;
    m_engine.GetContext().Add(IniString(conf.str()));
  }

  InstanceData result = {boost::polymorphic_downcast<aa::Strategy *>(
      &m_engine.GetContext().GetSrategy(strategyId->second))};

  result.strategy->Invoke<aa::Strategy>([this](aa::Strategy &advisor) {
    advisor.SetupAdvising(m_ui.highlightLevel->value() / 100);
    Assert(!advisor.GetAutoTradingSettings() ? true : false);
  });

  result.strategy->Invoke<aa::Strategy>([this, &result](aa::Strategy &advisor) {
    result.adviceConnection = advisor.SubscribeToAdvice(
        [this](const aa::Advice &advice) { emit Advice(advice); });
  });

  uint8_t biggestGetPricePrecision = 1;

  auto &context = m_engine.GetContext();
  for (size_t i = 0; i < context.GetNumberOfTradingSystems(); ++i) {
    auto &tradingSystem = context.GetTradingSystem(i, m_tradingMode);
    const auto &name = tradingSystem.GetInstanceName();
    const auto &key = boost::to_lower_copy(name);

    MarketDataSource *source = nullptr;
    for (size_t k = 0; i < context.GetNumberOfMarketDataSources(); ++k) {
      auto &sourceTmp = context.GetMarketDataSource(k);
      if (sourceTmp.GetInstanceName() == name) {
        source = &sourceTmp;
        break;
      }
    }
    if (!source) {
      QMessageBox::warning(
          this, tr("Configuration warning"),
          tr("Trading system \"%1\" does not have market data source.")
              .arg(QString::fromStdString(name)),
          QMessageBox::Ignore);
      continue;
    }

    const auto &widgets = m_targetWidgets.find(key);
    if (widgets == m_targetWidgets.cend()) {
      QMessageBox::warning(this, tr("Configuration warning"),
                           tr("Unknown trading system \"%1\".")
                               .arg(QString::fromStdString(name)),
                           QMessageBox::Ignore);
      continue;
    }
    result.*widgets->second->tradingSystemField = &tradingSystem;

    source->ForEachSecurity([&](Security &security) {
      if (security.GetSymbol().GetSymbol() != symbol) {
        return;
      }
      widgets->second->bid.SetPrecision(security.GetPricePrecision());
      widgets->second->ask.SetPrecision(security.GetPricePrecision());
      const Target target = {m_tradingMode, &security, &*widgets->second};
      Verify(result.targets.emplace(std::move(target)).second);
      if (biggestGetPricePrecision < security.GetPricePrecision()) {
        biggestGetPricePrecision = security.GetPricePrecision();
      }
    });
  }

  m_bestSpreadAbsValue =
      PriceAdapter<QLabel>{*m_ui.bestSpreadAbsValue, biggestGetPricePrecision};
  m_instanceData = std::move(result);
  m_symbol = symbolIndex;

  setWindowTitle(m_ui.symbol->itemText(symbolIndex) + " " + tr("Arbitrage") +
                 " - " + QCoreApplication::applicationName());
  {
    SignalsScopedBlocker blocker(*m_ui.autoTrade);
    m_ui.autoTrade->setChecked(false);
  }

  for (const auto &target : m_instanceData.targets) {
    target.widgets->Update(*target.security);
  }
}

void StrategyWindow::TakeAdvice(const aa::Advice &advice) {
  Assert(advice.security);

  {
    const auto &targetIndex = m_instanceData.targets.get<BySecurity>();
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
    const auto &targetIndex = m_instanceData.targets.get<BySecurity>();
    for (const auto &signal : advice.securitySignals) {
      const auto signalTargetIt = targetIndex.find(signal.security);
      Assert(signalTargetIt != targetIndex.cend());
      if (signalTargetIt == targetIndex.cend()) {
        continue;
      }
      setSideSignal(signalTargetIt->widgets->bid, signal.isBestBid,
                    *signal.security, m_instanceData.bestSellTradingSystem);
      setSideSignal(signalTargetIt->widgets->ask, signal.isBestAsk,
                    *signal.security, m_instanceData.bestBuyTradingSystem);
    }
  }
}

void StrategyWindow::SendOrder(const OrderSide &side,
                               TradingSystem *tradingSystem) {
  if (!tradingSystem) {
    tradingSystem = side == ORDER_SIDE_BUY
                        ? m_instanceData.bestBuyTradingSystem
                        : m_instanceData.bestSellTradingSystem;
    Assert(tradingSystem);
    if (!tradingSystem) {
      return;
    }
  }
  const auto &index = m_instanceData.targets.get<BySymbol>();
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
  }
}

bool StrategyWindow::IsAutoTradingActivated() const {
  bool result = false;
  if (m_instanceData.strategy) {
    m_instanceData.strategy->Invoke<aa::Strategy>(
        [this, &result](const aa::Strategy &advisor) {
          result = advisor.GetAutoTradingSettings() ? true : false;
        });
  }
  AssertEq(result, m_ui.autoTrade->isChecked());
  return result;
}

void StrategyWindow::ToggleAutoTrading(bool activate) {
  Assert(m_instanceData.strategy);
  try {
    m_instanceData.strategy->Invoke<aa::Strategy>(
        [this, activate](aa::Strategy &advisor) {
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
  if (!m_instanceData.strategy) {
    return;
  }
  try {
    m_instanceData.strategy->Invoke<aa::Strategy>(
        [](aa::Strategy &advisor) { advisor.DeactivateAutoTrading(); });
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to disable auto-trading"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  }
}

void StrategyWindow::UpdateAutoTradingLevel(double level) {
  Assert(m_instanceData.strategy);
  try {
    m_instanceData.strategy->Invoke<aa::Strategy>(
        [this, level](aa::Strategy &advisor) {
          if (!advisor.GetAutoTradingSettings()) {
            return;
          }
          advisor.ActivateAutoTrading({level / 100, m_ui.maxQty->value()});
        });
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, tr("Failed to setup auto-trading"),
                          QString("%1.").arg(ex.what()), QMessageBox::Abort);
  }
}

void StrategyWindow::UpdateAdviceLevel(double level) {
  Assert(m_instanceData.strategy);
  try {
    m_instanceData.strategy->Invoke<aa::Strategy>(
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
