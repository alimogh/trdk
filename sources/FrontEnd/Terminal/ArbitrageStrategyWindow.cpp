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
#include "ArbitrageStrategyWindow.hpp"
#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Strategies/ArbitrationAdvisor/Strategy.hpp"
#include "Lib/Engine.hpp"
#include "MainWindow.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Terminal;

namespace pt = boost::posix_time;
namespace ids = boost::uuids;
namespace aa = trdk::Strategies::ArbitrageAdvisor;

namespace {
size_t numberOfNextInstance = 1;
}

ArbitrageStrategyWindow::ArbitrageStrategyWindow(
    FrontEnd::Lib::Engine &engine,
    MainWindow &mainWindow,
    const boost::optional<QString> &defaultSymbol,
    QWidget *parent)
    : Base(parent),
      m_tradingMode(TRADING_MODE_LIVE),
      m_instanceNumber(numberOfNextInstance++),
      m_mainWindow(mainWindow),
      m_engine(engine),
      m_symbol(-1),
      m_instanceData({}) {
  setAttribute(Qt::WA_DeleteOnClose);

  m_ui.setupUi(this);

  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeLabel);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeLastTime);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeBid);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeAsk);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeSell);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeBuy);

  m_yobitnetWidgets.emplace_back(m_ui.yobitnetLabel);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetLastTime);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetBid);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetAsk);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetSell);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetBuy);

  m_ccexWidgets.emplace_back(m_ui.ccexLabel);
  m_ccexWidgets.emplace_back(m_ui.ccexLastTime);
  m_ccexWidgets.emplace_back(m_ui.ccexBid);
  m_ccexWidgets.emplace_back(m_ui.ccexAsk);
  m_ccexWidgets.emplace_back(m_ui.ccexSell);
  m_ccexWidgets.emplace_back(m_ui.ccexBuy);

  m_gdaxWidgets.emplace_back(m_ui.gdaxLabel);
  m_gdaxWidgets.emplace_back(m_ui.gdaxLastTime);
  m_gdaxWidgets.emplace_back(m_ui.gdaxBid);
  m_gdaxWidgets.emplace_back(m_ui.gdaxAsk);
  m_gdaxWidgets.emplace_back(m_ui.gdaxSell);
  m_gdaxWidgets.emplace_back(m_ui.gdaxBuy);

  Verify(connect(m_ui.symbol, static_cast<void (QComboBox::*)(int)>(
                                  &QComboBox::currentIndexChanged),
                 this, &ArbitrageStrategyWindow::OnCurrentSymbolChange));

  Verify(connect(m_ui.highlightLevel,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 this, &ArbitrageStrategyWindow::UpdateAdviceLevel));

  Verify(connect(m_ui.autoTrade, &QCheckBox::toggled, this,
                 &ArbitrageStrategyWindow::ToggleAutoTrading));
  Verify(connect(m_ui.autoTradeLevel,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 this, &ArbitrageStrategyWindow::UpdateAutoTradingLevel));

  Verify(connect(m_ui.novaexchangeSell, &QPushButton::clicked, [this]() {
    Assert(m_instanceData.novaexchangeTradingSystem);
    Sell(*m_instanceData.novaexchangeTradingSystem);
  }));
  Verify(connect(m_ui.novaexchangeBuy, &QPushButton::clicked, [this]() {
    Assert(m_instanceData.novaexchangeTradingSystem);
    Buy(*m_instanceData.novaexchangeTradingSystem);
  }));
  Verify(connect(m_ui.yobitnetSell, &QPushButton::clicked, [this]() {
    Assert(m_instanceData.yobitnetTradingSystem);
    Sell(*m_instanceData.yobitnetTradingSystem);
  }));
  Verify(connect(m_ui.yobitnetBuy, &QPushButton::clicked, [this]() {
    Assert(m_instanceData.yobitnetTradingSystem);
    Buy(*m_instanceData.yobitnetTradingSystem);
  }));
  Verify(connect(m_ui.ccexSell, &QPushButton::clicked, [this]() {
    Assert(m_instanceData.ccexTradingSystem);
    Sell(*m_instanceData.ccexTradingSystem);
  }));
  Verify(connect(m_ui.ccexBuy, &QPushButton::clicked, [this]() {
    Assert(m_instanceData.ccexTradingSystem);
    Buy(*m_instanceData.ccexTradingSystem);
  }));
  Verify(connect(m_ui.gdaxSell, &QPushButton::clicked, [this]() {
    Assert(m_instanceData.gdaxTradingSystem);
    Sell(*m_instanceData.gdaxTradingSystem);
  }));
  Verify(connect(m_ui.gdaxBuy, &QPushButton::clicked, [this]() {
    Assert(m_instanceData.gdaxTradingSystem);
    Buy(*m_instanceData.gdaxTradingSystem);
  }));

  qRegisterMetaType<trdk::Strategies::ArbitrageAdvisor::Advice>(
      "trdk::Strategies::ArbitrageAdvisor::Advice");
  Verify(connect(this, &ArbitrageStrategyWindow::Advice, this,
                 &ArbitrageStrategyWindow::TakeAdvice, Qt::QueuedConnection));

  LoadSymbols(defaultSymbol);

  adjustSize();
}

ArbitrageStrategyWindow::~ArbitrageStrategyWindow() {
  try {
    DeactivateAutoTrading();
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

void ArbitrageStrategyWindow::closeEvent(QCloseEvent *closeEvent) {
  if (!IsAutoTradingActivated()) {
    closeEvent->accept();
    return;
  }
  const auto &response = QMessageBox::question(
      this, tr("Strategy closing"),
      tr("Are you sure you want to the close strategy window?\n\nIf strategy "
         "window will be closed - automatic trading will be "
         "stopped.\n\nContinue and close strategy window for %1?")
          .arg(m_ui.symbol->itemText(m_symbol)),
      QMessageBox::Yes | QMessageBox::No);
  if (response == QMessageBox::Yes) {
    closeEvent->accept();
  } else {
    closeEvent->ignore();
  }
}

QSize ArbitrageStrategyWindow::sizeHint() const {
  auto result = Base::sizeHint();
  result.setWidth(static_cast<int>(result.width() * 0.9));
  result.setHeight(static_cast<int>(result.height() * 0.7));
  return result;
}

void ArbitrageStrategyWindow::LoadSymbols(
    const boost::optional<QString> &defaultSymbol) {
  {
    const SignalsScopedBlocker blocker(*m_ui.symbol);
    m_ui.symbol->addItem("ETH_BTC");
    m_ui.symbol->addItem("ETC_BTC");
    m_ui.symbol->addItem("DOGE_BTC");
    m_ui.symbol->addItem("LTC_BTC");
    m_ui.symbol->addItem("CNT_BTC");
    m_ui.symbol->addItem("DASH_BTC");
  }
  if (defaultSymbol) {
    m_ui.symbol->setCurrentText(*defaultSymbol);
  } else {
    SetCurrentSymbol(m_ui.symbol->currentIndex());
  }
}

void ArbitrageStrategyWindow::OnCurrentSymbolChange(int newSymbolIndex) {
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
          m_mainWindow.CreateNewArbitrageStrategy(symbol);
        }
        return;
      }

      m_ui.symbol->setCurrentIndex(newSymbolIndex);
    }
  }
  SetCurrentSymbol(newSymbolIndex);
}

void ArbitrageStrategyWindow::SetCurrentSymbol(int symbolIndex) {
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
    Assert(!advisor.GetAutoTradingMinPriceDifferenceRatio() ? true : false);
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

    QLabel *bidPriceWidget;
    QLabel *bidQtyWidget;
    QLabel *askPriceWidget;
    QLabel *askQtyWidget;
    QLabel *lastTimeWidget;
    QFrame *bidFrame;
    QFrame *askFrame;
    if (boost::iequals(name, "Novaexchange")) {
      result.novaexchangeTradingSystem = &tradingSystem;
      bidPriceWidget = m_ui.novaexchangeBidPrice;
      bidQtyWidget = m_ui.novaexchangeBidQty;
      askPriceWidget = m_ui.novaexchangeAskPrice;
      askQtyWidget = m_ui.novaexchangeAskQty;
      lastTimeWidget = m_ui.novaexchangeLastTime;
      bidFrame = m_ui.novaexchangeBid;
      askFrame = m_ui.novaexchangeAsk;
    } else if (boost::iequals(name, "Yobitnet")) {
      result.yobitnetTradingSystem = &tradingSystem;
      bidPriceWidget = m_ui.yobitnetBidPrice;
      bidQtyWidget = m_ui.yobitnetBidQty;
      askPriceWidget = m_ui.yobitnetAskPrice;
      askQtyWidget = m_ui.yobitnetAskQty;
      lastTimeWidget = m_ui.yobitnetLastTime;
      bidFrame = m_ui.yobitnetBid;
      askFrame = m_ui.yobitnetAsk;
    } else if (boost::iequals(name, "Ccex")) {
      result.ccexTradingSystem = &tradingSystem;
      bidPriceWidget = m_ui.ccexBidPrice;
      bidQtyWidget = m_ui.ccexBidQty;
      askPriceWidget = m_ui.ccexAskPrice;
      askQtyWidget = m_ui.ccexAskQty;
      lastTimeWidget = m_ui.ccexLastTime;
      bidFrame = m_ui.ccexBid;
      askFrame = m_ui.ccexAsk;
    } else {
      QMessageBox::warning(this, tr("Configuration warning"),
                           tr("Unknown trading system \"%1\".")
                               .arg(QString::fromStdString(name)),
                           QMessageBox::Ignore);
      continue;
    }

    source->ForEachSecurity([&](Security &security) {
      if (security.GetSymbol().GetSymbol() != symbol) {
        return;
      }
      const Target target = {&security,
                             SideAdapter<QLabel>{*bidPriceWidget, *bidQtyWidget,
                                                 security.GetPricePrecision()},
                             SideAdapter<QLabel>{*askPriceWidget, *askQtyWidget,
                                                 security.GetPricePrecision()},
                             TimeAdapter<QLabel>{*lastTimeWidget},
                             bidFrame,
                             askFrame};
      Verify(result.targets.emplace(std::move(target)).second);
      if (biggestGetPricePrecision < security.GetPricePrecision()) {
        biggestGetPricePrecision = security.GetPricePrecision();
      }
    });
  }

  m_bestSpreadAbsValue = Lib::PriceAdapter<QLabel>{*m_ui.bestSpreadAbsValue,
                                                   biggestGetPricePrecision};
  m_instanceData = std::move(result);
  m_symbol = symbolIndex;

  setWindowTitle(m_ui.symbol->itemText(symbolIndex) + " " + tr("Arbitrage") +
                 " - " + QCoreApplication::applicationName());
  {
    SignalsScopedBlocker blocker(*m_ui.autoTrade);
    m_ui.autoTrade->setChecked(false);
  }
  for (auto *const widget : m_novaexchangeWidgets) {
    widget->setEnabled(m_instanceData.novaexchangeTradingSystem ? true : false);
  }
  for (auto *const widget : m_yobitnetWidgets) {
    widget->setEnabled(m_instanceData.yobitnetTradingSystem ? true : false);
  }
  for (auto *const widget : m_ccexWidgets) {
    widget->setEnabled(m_instanceData.ccexTradingSystem ? true : false);
  }
  for (auto *const widget : m_gdaxWidgets) {
    widget->setEnabled(m_instanceData.gdaxTradingSystem ? true : false);
  }
  for (const auto &target : m_instanceData.targets) {
    const auto &security = *target.security;
    target.bid.Set(security.GetBidPriceValue(), security.GetBidQtyValue());
    target.ask.Set(security.GetAskPriceValue(), security.GetAskQtyValue());
    target.time.Set(security.GetLastMarketDataTime());
  }
}

void ArbitrageStrategyWindow::TakeAdvice(const aa::Advice &advice) {
  Assert(advice.security);

  {
    const auto &targetIndex = m_instanceData.targets.get<BySecurity>();
    const auto updateTargeIt = targetIndex.find(advice.security);
    Assert(updateTargeIt != targetIndex.cend());
    if (updateTargeIt == targetIndex.cend()) {
      return;
    }
    updateTargeIt->time.Set(advice.time);
    updateTargeIt->bid.Set(advice.bid.price, advice.bid.qty);
    updateTargeIt->ask.Set(advice.ask.price, advice.ask.qty);
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
        bool isSignaled, QFrame &frame, const QString &signaledStyle,
        const QString &highlightedStyle) {
      if (isSignaled) {
        if (advice.isSignaled) {
          frame.setStyleSheet(signaledStyle);
        } else {
          frame.setStyleSheet(highlightedStyle);
        }
      } else {
        frame.setStyleSheet(styleSheet());
      }
    };
    const auto &targetIndex = m_instanceData.targets.get<BySecurity>();
    for (const aa::Advice::SecuritySignal &signal : advice.securitySignals) {
      const auto signalTargetIt = targetIndex.find(signal.security);
      Assert(signalTargetIt != targetIndex.cend());
      if (signalTargetIt == targetIndex.cend()) {
        continue;
      }
      {
        static const QString signaledStyle(
            "background-color: rgb(230, 59, 1);");
        static const QString highlightedStyle("color: rgb(230, 59, 1);");
        setSideSignal(signal.isBidSignaled, *signalTargetIt->bidFrame,
                      signaledStyle, highlightedStyle);
      }
      {
        static const QString signaledStyle(
            "background-color: rgb(0, 146, 68);");
        static const QString highlightedStyle("color: rgb(0, 195, 91);");
        setSideSignal(signal.isAskSignaled, *signalTargetIt->askFrame,
                      signaledStyle, highlightedStyle);
      }
    }
  }
}

void ArbitrageStrategyWindow::Sell(TradingSystem &tradingSystem) {
  tradingSystem;
}

void ArbitrageStrategyWindow::Buy(TradingSystem &tradingSystem) {
  tradingSystem;
}

bool ArbitrageStrategyWindow::IsAutoTradingActivated() const {
  bool result = false;
  if (!m_instanceData.strategy) {
    m_instanceData.strategy->Invoke<aa::Strategy>([this, &result](
        const aa::Strategy &advisor) {
      result = advisor.GetAutoTradingMinPriceDifferenceRatio() ? true : false;
    });
  }
  AssertEq(result, m_ui.autoTrade->isChecked());
  return result;
}

void ArbitrageStrategyWindow::ToggleAutoTrading(bool activate) {
  Assert(m_instanceData.strategy);
  m_instanceData.strategy->Invoke<aa::Strategy>([this, activate](
      aa::Strategy &advisor) {
    AssertNe(activate,
             advisor.GetAutoTradingMinPriceDifferenceRatio() ? true : false);
    activate ? advisor.ActivateAutoTrading(m_ui.autoTradeLevel->value() / 100)
             : advisor.DeactivateAutoTrading();
  });
}

void ArbitrageStrategyWindow::DeactivateAutoTrading() {
  if (!m_instanceData.strategy) {
    return;
  }
  m_instanceData.strategy->Invoke<aa::Strategy>(
      [](aa::Strategy &advisor) { advisor.DeactivateAutoTrading(); });
}

void ArbitrageStrategyWindow::UpdateAutoTradingLevel(double level) {
  Assert(m_instanceData.strategy);
  m_instanceData.strategy->Invoke<aa::Strategy>([level](aa::Strategy &advisor) {
    if (!advisor.GetAutoTradingMinPriceDifferenceRatio()) {
      return;
    }
    advisor.ActivateAutoTrading(level / 100);
  });
}

void ArbitrageStrategyWindow::UpdateAdviceLevel(double level) {
  Assert(m_instanceData.strategy);
  m_instanceData.strategy->Invoke<aa::Strategy>(
      [level](aa::Strategy &advisor) { advisor.SetupAdvising(level / 100); });
}