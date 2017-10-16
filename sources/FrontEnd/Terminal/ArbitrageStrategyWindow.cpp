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
#include "Lib/DropCopy.hpp"
#include "Lib/Engine.hpp"
#include "MainWindow.hpp"

using namespace trdk;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Terminal;

ArbitrageStrategyWindow::ArbitrageStrategyWindow(
    FrontEnd::Lib::Engine &engine,
    MainWindow &mainWindow,
    const boost::optional<QString> &defaultSymbol,
    QWidget *parent)
    : QMainWindow(parent),
      m_mainWindow(mainWindow),
      m_tradingMode(TRADING_MODE_LIVE),
      m_engine(engine),
      m_currentSymbol(-1),
      m_instanceData({}) {
  m_ui.setupUi(this);
  setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
                 Qt::WindowSystemMenuHint);

  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeLabel);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeLastTime);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeBidPrice);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeBidQty);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeAskPrice);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeAskQty);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeSell);
  m_novaexchangeWidgets.emplace_back(m_ui.novaexchangeBuy);

  m_yobitnetWidgets.emplace_back(m_ui.yobitnetLabel);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetLastTime);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetBidPrice);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetBidQty);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetAskPrice);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetAskQty);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetSell);
  m_yobitnetWidgets.emplace_back(m_ui.yobitnetBuy);

  m_ccexWidgets.emplace_back(m_ui.ccexLabel);
  m_ccexWidgets.emplace_back(m_ui.ccexLastTime);
  m_ccexWidgets.emplace_back(m_ui.ccexBidPrice);
  m_ccexWidgets.emplace_back(m_ui.ccexBidQty);
  m_ccexWidgets.emplace_back(m_ui.ccexAskPrice);
  m_ccexWidgets.emplace_back(m_ui.ccexAskQty);
  m_ccexWidgets.emplace_back(m_ui.ccexSell);
  m_ccexWidgets.emplace_back(m_ui.ccexBuy);

  m_gdaxWidgets.emplace_back(m_ui.gdaxLabel);
  m_gdaxWidgets.emplace_back(m_ui.gdaxLastTime);
  m_gdaxWidgets.emplace_back(m_ui.gdaxBidPrice);
  m_gdaxWidgets.emplace_back(m_ui.gdaxBidQty);
  m_gdaxWidgets.emplace_back(m_ui.gdaxAskPrice);
  m_gdaxWidgets.emplace_back(m_ui.gdaxAskQty);
  m_gdaxWidgets.emplace_back(m_ui.gdaxSell);
  m_gdaxWidgets.emplace_back(m_ui.gdaxBuy);

  Verify(connect(m_ui.symbol, static_cast<void (QComboBox::*)(int)>(
                                  &QComboBox::currentIndexChanged),
                 this, &ArbitrageStrategyWindow::OnCurrentSymbolChange));

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

  Verify(connect(&m_engine.GetDropCopy(), &Lib::DropCopy::PriceUpdate, this,
                 &ArbitrageStrategyWindow::UpdatePrices, Qt::QueuedConnection));

  LoadSymbols(defaultSymbol);
}

void ArbitrageStrategyWindow::LoadSymbols(
    const boost::optional<QString> &defaultSymbol) {
  int defaultSymbolIndex = 0;

  boost::unordered_set<std::string> result;
  auto &context = m_engine.GetContext();
  for (size_t i = 0; i < context.GetNumberOfMarketDataSources(); ++i) {
    context.GetMarketDataSource(i).ForEachSecurity([&](Security &security) {
      result.emplace(security.GetSymbol().GetSymbol());
    });
  }

  const SignalsScopedBlocker signalsBlocker(*m_ui.symbol);
  m_ui.symbol->clear();
  for (const auto &symbol : result) {
    const auto &symbolQStr = QString::fromStdString(symbol);
    if (defaultSymbol && symbolQStr == *defaultSymbol) {
      defaultSymbolIndex = m_ui.symbol->count();
    }
    m_ui.symbol->addItem(std::move(symbolQStr));
  }
  if (!m_ui.symbol->count()) {
    return;
  }
  m_ui.symbol->setCurrentIndex(defaultSymbolIndex);
  SetCurrentSymbol(defaultSymbolIndex);
}

void ArbitrageStrategyWindow::OnCurrentSymbolChange(int newSymbolIndex) {
  const QString &symbol = m_ui.symbol->itemText(newSymbolIndex);
  if (m_currentSymbol >= 0) {
    if (m_currentSymbol == newSymbolIndex) {
      return;
    }
    const auto &response = QMessageBox::question(
        this, tr("Strategy symbol change"),
        tr("Are you sure you want to change the symbol of strategy from %1 to "
           "%2?\n\nClick Yes to continue and change symbol for this strategy "
           "instance, choose No to start new strategy instance for %2, or "
           "click Cancel to cancel the action.")
            .arg(m_ui.symbol->itemText(m_currentSymbol), symbol),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (response != QMessageBox::Yes) {
      if (response == QMessageBox::No) {
        m_mainWindow.CreateNewArbitrageStrategy(symbol);
      }
      const SignalsScopedBlocker blocker(*m_ui.symbol);
      m_ui.symbol->setCurrentIndex(m_currentSymbol);
    }
  }
  SetCurrentSymbol(newSymbolIndex);
}

void ArbitrageStrategyWindow::SetCurrentSymbol(int symbolIndex) {
  m_currentSymbol = symbolIndex;

  setWindowTitle(m_ui.symbol->itemText(m_currentSymbol) + " " +
                 tr("Arbitrage") + " - " + QCoreApplication::applicationName());

  const std::string symbol =
      m_ui.symbol->itemText(m_currentSymbol).toStdString();

  InstanceData result = {};

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
    if (boost::iequals(name, "Novaexchange")) {
      result.novaexchangeTradingSystem = &tradingSystem;
      bidPriceWidget = m_ui.novaexchangeBidPrice;
      bidQtyWidget = m_ui.novaexchangeBidQty;
      askPriceWidget = m_ui.novaexchangeAskPrice;
      askQtyWidget = m_ui.novaexchangeAskQty;
      lastTimeWidget = m_ui.novaexchangeLastTime;
    } else if (boost::iequals(name, "Yobitnet")) {
      result.yobitnetTradingSystem = &tradingSystem;
      bidPriceWidget = m_ui.yobitnetBidPrice;
      bidQtyWidget = m_ui.yobitnetBidQty;
      askPriceWidget = m_ui.yobitnetAskPrice;
      askQtyWidget = m_ui.yobitnetAskQty;
      lastTimeWidget = m_ui.yobitnetLastTime;
    } else if (boost::iequals(name, "Ccex")) {
      result.ccexTradingSystem = &tradingSystem;
      bidPriceWidget = m_ui.ccexBidPrice;
      bidQtyWidget = m_ui.ccexBidQty;
      askPriceWidget = m_ui.ccexAskPrice;
      askQtyWidget = m_ui.ccexAskQty;
      lastTimeWidget = m_ui.ccexLastTime;
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
      const Target target = {&tradingSystem, &security,
                             SideAdapter<QLabel>{*bidPriceWidget, *bidQtyWidget,
                                                 security.GetPricePrecision()},
                             SideAdapter<QLabel>{*askPriceWidget, *askQtyWidget,
                                                 security.GetPricePrecision()},
                             TimeAdapter<QLabel>{*lastTimeWidget}};
      Verify(result.targets.emplace(std::move(target)).second);
    });
  }

  m_instanceData = std::move(result);

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
    UpdateTargetPrices(target);
  }
}

void ArbitrageStrategyWindow::UpdatePrices(const Security *security) {
  Assert(security);
  const auto &index = m_instanceData.targets.get<BySecurity>();
  const auto &it = index.find(security);
  if (it == index.cend()) {
    return;
  }
  UpdateTargetPrices(*it);
}

void ArbitrageStrategyWindow::UpdateTargetPrices(const Target &target) {
  const auto &security = *target.security;
  target.bid.Set(security.GetBidPriceValue(), security.GetBidQtyValue());
  target.ask.Set(security.GetAskPriceValue(), security.GetAskQtyValue());
  target.time.Set(security.GetLastMarketDataTime());
}

void ArbitrageStrategyWindow::Sell(TradingSystem &tradingSystem) {
  tradingSystem;
}

void ArbitrageStrategyWindow::Buy(TradingSystem &tradingSystem) {
  tradingSystem;
}
