/*******************************************************************************
 *   Created: 2017/10/01 18:36:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "MultiBrokerWidget.hpp"
#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Strategies/WilliamCarry/WilliamCarryMultibroker.hpp"
#include "Strategies/WilliamCarry/WilliamCarryOperationContext.hpp"
#include "Lib/DropCopy.hpp"
#include "Lib/Engine.hpp"
#include "ShellLib/ShellModule.hpp"
#include "TimersDialog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::WilliamCarry;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Shell;
using namespace trdk::FrontEnd::WilliamCarry;

namespace pt = boost::posix_time;
namespace ids = boost::uuids;

MultiBrokerWidget::MultiBrokerWidget(Engine &engine, QWidget *parent)
    : QWidget(parent),
      m_mode(TRADING_MODE_LIVE),
      m_engine(engine),
      m_currentTradingSecurity(nullptr),
      m_currentMarketDataSecurity(nullptr),
      m_tradingsecurityListWidget(this),
      m_ignoreLockToggling(false) {
  m_ui.setupUi(this);

  m_tradingsecurityListWidget.hide();
  m_tradingsecurityListWidget.raise();

  setEnabled(false);

  Verify(connect(m_ui.lock, &QPushButton::toggled, this,
                 &MultiBrokerWidget::LockSecurity));

  Verify(connect(m_ui.buy1, &QPushButton::clicked,
                 [this]() { OpenPosition(0, true); }));
  Verify(connect(m_ui.sell1, &QPushButton::clicked,
                 [this]() { OpenPosition(0, false); }));
  Verify(connect(m_ui.buy2, &QPushButton::clicked,
                 [this]() { OpenPosition(1, true); }));
  Verify(connect(m_ui.sell2, &QPushButton::clicked,
                 [this]() { OpenPosition(1, false); }));
  Verify(connect(m_ui.buy3, &QPushButton::clicked,
                 [this]() { OpenPosition(2, true); }));
  Verify(connect(m_ui.sell3, &QPushButton::clicked,
                 [this]() { OpenPosition(2, false); }));
  Verify(connect(m_ui.buy4, &QPushButton::clicked,
                 [this]() { OpenPosition(3, true); }));
  Verify(connect(m_ui.sell4, &QPushButton::clicked,
                 [this]() { OpenPosition(3, false); }));
  Verify(connect(m_ui.closeAll, &QPushButton::clicked,
                 [this]() { CloseAllPositions(); }));
  Verify(connect(m_ui.currentTradingSecurity, &QPushButton::clicked, this,
                 &MultiBrokerWidget::ShowTradingSecurityList));

  Verify(connect(m_ui.showTimers, &QPushButton::clicked, this,
                 &MultiBrokerWidget::ShowTimersSetupDialog));

  Verify(connect(m_ui.targetList, &QListWidget::currentRowChanged,
                 [this](int index) {
                   if (index < 0) {
                     m_ui.securityList->setCurrentRow(-1);
                     m_tradingsecurityListWidget.setCurrentIndex(-1);
                     return;
                   }
                   ReloadSecurityList();
                 }));
  Verify(connect(m_ui.securityList, &QListWidget::currentRowChanged,
                 [this](int index) {
                   if (index < 0) {
                     SetMarketDataSecurity(nullptr);
                     return;
                   }
                   AssertGt(m_securityList.size(), index);
                   SetMarketDataSecurity(&*m_securityList[index]);
                 }));

  Verify(connect(
      &m_tradingsecurityListWidget,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      [this](int index) {
        if (index < 0) {
          SetTradingSecurity(nullptr);
          return;
        }
        AssertGt(m_securityList.size(), index);
        SetTradingSecurity(&*m_securityList[index]);
        m_tradingsecurityListWidget.hide();
      }));

  Verify(connect(&m_engine, &Engine::StateChanged, this,
                 &MultiBrokerWidget::OnStateChanged, Qt::QueuedConnection));

  Verify(connect(&m_engine.GetDropCopy(), &Lib::DropCopy::PriceUpdate, this,
                 &MultiBrokerWidget::UpdatePrices, Qt::QueuedConnection));
}

void MultiBrokerWidget::resizeEvent(QResizeEvent *) {
  m_tradingsecurityListWidget.hide();
}

void MultiBrokerWidget::OpenPosition(size_t strategyIndex, bool isLong) {
  AssertGt(m_strategies.size(), strategyIndex);
  Assert(m_strategies[strategyIndex]);
  Assert(m_currentTradingSecurity);

  for (;;) {
    const auto &delayMeasurement =
        m_engine.GetContext().StartStrategyTimeMeasurement();
    OperationContext operation(isLong, 1);
    try {
      m_strategies[strategyIndex]->Invoke<Multibroker>(
          [this, &operation, &delayMeasurement](Multibroker &strategy) {
            strategy.OpenPosition(std::move(operation),
                                  *m_currentTradingSecurity, delayMeasurement);
          });
      break;
    } catch (const std::exception &ex) {
      if (QMessageBox::critical(this, tr("Failed to open new position"),
                                QString("%1.").arg(ex.what()),
                                QMessageBox::Abort | QMessageBox::Retry) !=
          QMessageBox::Retry) {
        break;
      }
    }
  }
}

void MultiBrokerWidget::CloseAllPositions() {
  for (;;) {
    try {
      m_engine.GetContext().CloseSrategiesPositions();
      break;
    } catch (const std::exception &ex) {
      if (QMessageBox::critical(this, tr("Failed to close all positions"),
                                QString("%1.").arg(ex.what()),
                                QMessageBox::Abort | QMessageBox::Retry) !=
          QMessageBox::Retry) {
        break;
      }
    }
  }
}

void MultiBrokerWidget::LockSecurity(bool lock) {
  if (m_ignoreLockToggling) {
    return;
  }
  if (!lock) {
    if (m_currentMarketDataSecurity) {
      m_lockedSecurityList.erase(m_currentMarketDataSecurity);
    }
    ResetLockedPrices(m_currentMarketDataSecurity);
    return;
  }

  Assert(m_currentMarketDataSecurity);
  const auto &locked = m_lockedSecurityList.emplace(std::make_pair(
      m_currentMarketDataSecurity,
      Locked{m_currentMarketDataSecurity->GetLastMarketDataTime(),
             m_currentMarketDataSecurity->GetBidPriceValue(),
             m_currentMarketDataSecurity->GetAskPriceValue()}));
  Assert(locked.second);
  SetLockedPrices(locked.first->second, *m_currentMarketDataSecurity);
}

TradingSystem *MultiBrokerWidget::GetSelectedTradingSystem() {
  const auto &index = m_ui.targetList->currentRow();
  if (index < 0) {
    return nullptr;
  }
  return &m_engine.GetContext().GetTradingSystem(index, m_mode);
}

void MultiBrokerWidget::Reload() {
  m_securityList.clear();
  m_ui.securityList->clear();
  m_tradingsecurityListWidget.clear();
  m_ui.targetList->clear();
  m_lockedSecurityList.clear();
  LockSecurity(false);

  for (size_t i = 0; i < m_engine.GetContext().GetNumberOfTradingSystems();
       ++i) {
    auto &tradingSystem = m_engine.GetContext().GetTradingSystem(i, m_mode);
    m_ui.targetList->addItem(
        QString::fromStdString(tradingSystem.GetInstanceName()));
  }
  ReloadSecurityList();
  if (m_ui.targetList->count() > 0) {
    m_ui.targetList->setCurrentRow(0);
  }

  m_strategies[0] = &m_engine.GetContext().GetSrategy(
      ids::string_generator()("DB005668-9EA5-42E2-88CD-8B25040CDE34"));
  m_strategies[1] = &m_engine.GetContext().GetSrategy(
      ids::string_generator()("D7E1F34F-A6A8-4656-9612-8D9F93D62760"));
  m_strategies[2] = &m_engine.GetContext().GetSrategy(
      ids::string_generator()("646CE7C2-216B-4C2F-A7B8-5B8B45B0EBF4"));
  m_strategies[3] = &m_engine.GetContext().GetSrategy(
      ids::string_generator()("A7ACF9B5-B3FB-4DD8-BA39-CE7DD04D4F2E"));
}

void MultiBrokerWidget::ReloadSecurityList() {
  const auto *const tradingSystem = GetSelectedTradingSystem();
  if (!tradingSystem) {
    m_ui.securityList->clear();
    m_tradingsecurityListWidget.clear();
    return;
  }

  m_ui.securityList->blockSignals(true);
  m_tradingsecurityListWidget.blockSignals(true);
  m_ui.securityList->clear();
  m_tradingsecurityListWidget.clear();

  int tradingSecurity = 0;
  int marketDataSecurity = 0;
  for (size_t i = 0; i < m_engine.GetContext().GetNumberOfMarketDataSources();
       ++i) {
    auto &source = m_engine.GetContext().GetMarketDataSource(i);
    if (source.GetInstanceName() != tradingSystem->GetInstanceName()) {
      continue;
    }
    source.ForEachSecurity([&](Security &security) {
      m_securityList.emplace_back(&security);
      if (m_currentTradingSecurity &&
          m_currentTradingSecurity->GetSymbol() == security.GetSymbol()) {
        tradingSecurity = m_tradingsecurityListWidget.count();
      }
      if (m_currentMarketDataSecurity &&
          m_currentMarketDataSecurity->GetSymbol() == security.GetSymbol()) {
        marketDataSecurity = m_ui.securityList->count();
      }
      {
        const auto symbol =
#ifdef _DEBUG
            QString("%1 (%2)").arg(
                QString::fromStdString(security.GetSymbol().GetSymbol()),
                QString::fromStdString(security.GetSource().GetInstanceName()))
#else
            QString::fromStdString(security.GetSymbol().GetSymbol())
#endif
            ;
        m_ui.securityList->addItem(symbol);
        m_tradingsecurityListWidget.addItem(symbol);
      }
    });
    break;
  }
  m_ui.securityList->blockSignals(false);
  m_tradingsecurityListWidget.blockSignals(false);

  m_ui.securityList->setCurrentRow(marketDataSecurity);
  m_tradingsecurityListWidget.setCurrentIndex(tradingSecurity);
  SetTradingSecurity(&*m_securityList[tradingSecurity]);
}

void MultiBrokerWidget::OnStateChanged(bool isStarted) {
  if (!isEnabled() && isStarted) {
    Reload();
  }
  setEnabled(isStarted);
}

void MultiBrokerWidget::UpdatePrices(const Security *security) {
  if (m_currentMarketDataSecurity != security) {
    return;
  }
  SetCurretPrices(security->GetLastMarketDataTime(),
                  security->GetBidPriceValue(), security->GetAskPriceValue(),
                  security);
}

void MultiBrokerWidget::SetCurretPrices(const pt::ptime &time,
                                        const Price &bid,
                                        const Price &ask,
                                        const Security *security) {
  SetPrices(time, *m_ui.time, bid, *m_ui.bid, ask, *m_ui.ask, security);
}

void MultiBrokerWidget::SetLockedPrices(const Locked &locked,
                                        const Security &security) {
  SetPrices(locked.time, *m_ui.lockedTime, locked.bid, *m_ui.lockedBid,
            locked.ask, *m_ui.lockedAsk, &security);
}
void MultiBrokerWidget::ResetLockedPrices(const Security *security) {
  SetPrices(pt::not_a_date_time, *m_ui.lockedTime,
            std::numeric_limits<double>::quiet_NaN(), *m_ui.lockedBid,
            std::numeric_limits<double>::quiet_NaN(), *m_ui.lockedAsk,
            security);
}

void MultiBrokerWidget::SetPrices(const pt::ptime &time,
                                  QLabel &timeControl,
                                  const Price &bid,
                                  QLabel &bidControl,
                                  const Price &ask,
                                  QLabel &askControl,
                                  const Security *security) const {
  if (time != pt::not_a_date_time) {
    const auto &timeOfDay = time.time_of_day();
    QString text;
    text.sprintf("%02d:%02d:%02d", timeOfDay.hours(), timeOfDay.minutes(),
                 timeOfDay.seconds());
    timeControl.setText(text);
  } else {
    timeControl.setText(QString());
  }
  {
    const auto &precision = security ? security->GetPricePrecision() : 5;
    if (!isnan(bid)) {
      bidControl.setText(QString::number(bid, 'f', precision));
    } else {
      bidControl.setText(QString());
    }
    if (!isnan(ask)) {
      askControl.setText(QString::number(ask, 'f', precision));
    } else {
      askControl.setText(QString());
    }
  }
}

void MultiBrokerWidget::ShowTimersSetupDialog() {
  TimersDialog dialog(this);

  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
}

void MultiBrokerWidget::ShowTradingSecurityList() {
  m_tradingsecurityListWidget.move(
      m_ui.currentTradingSecurity->geometry().topLeft());
  m_tradingsecurityListWidget.setMinimumWidth(
      m_ui.currentTradingSecurity->width());
  m_tradingsecurityListWidget.show();
  m_tradingsecurityListWidget.showPopup();
  m_tradingsecurityListWidget.setFocus();
}

void MultiBrokerWidget::SetTradingSecurity(Security *security) {
  m_currentTradingSecurity = security;

  m_ui.currentTradingSecurity->setText(
      !m_currentTradingSecurity
          ? QString()
          : QString::fromStdString(security->GetSymbol().GetSymbol()));

  m_ui.buy1->setEnabled(m_currentTradingSecurity ? true : false);
  m_ui.sell1->setEnabled(m_currentTradingSecurity ? true : false);
  m_ui.buy2->setEnabled(m_currentTradingSecurity ? true : false);
  m_ui.sell2->setEnabled(m_currentTradingSecurity ? true : false);
  m_ui.buy3->setEnabled(m_currentTradingSecurity ? true : false);
  m_ui.sell3->setEnabled(m_currentTradingSecurity ? true : false);
  m_ui.buy4->setEnabled(m_currentTradingSecurity ? true : false);
  m_ui.sell4->setEnabled(m_currentTradingSecurity ? true : false);
}

void MultiBrokerWidget::SetMarketDataSecurity(Security *security) {
  m_currentMarketDataSecurity = security;

  m_ui.lock->setEnabled(m_currentMarketDataSecurity ? true : false);

  if (!m_currentMarketDataSecurity) {
    m_ui.symbol->setText(QString());
    return;
  }

  m_ui.symbol->setText(QString::fromStdString(
      m_currentMarketDataSecurity->GetSymbol().GetSymbol()));
  UpdatePrices(m_currentMarketDataSecurity);

  {
    const auto &locked = m_lockedSecurityList.find(m_currentMarketDataSecurity);
    Assert(!m_ignoreLockToggling);
    m_ignoreLockToggling = true;
    if (locked == m_lockedSecurityList.cend()) {
      ResetLockedPrices(m_currentMarketDataSecurity);
      m_ui.lock->setChecked(false);
    } else {
      SetLockedPrices(locked->second, *m_currentMarketDataSecurity);
      m_ui.lock->setChecked(true);
    }
    Assert(m_ignoreLockToggling);
    m_ignoreLockToggling = false;
  }
}

////////////////////////////////////////////////////////////////////////////////

ModuleFactoryResult CreateEngineFrontEndWidgets(Engine &engine,
                                                QWidget *parent) {
  ModuleFactoryResult result;
  result.emplace_back("Multi Broker",
                      boost::make_unique<MultiBrokerWidget>(engine, parent));
  return result;
}

////////////////////////////////////////////////////////////////////////////////