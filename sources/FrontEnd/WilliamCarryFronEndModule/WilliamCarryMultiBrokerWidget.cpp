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
#include "WilliamCarryMultiBrokerWidget.hpp"
#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Strategies/WilliamCarry/WilliamCarryMultibroker.hpp"
#include "Strategies/WilliamCarry/WilliamCarryOperationContext.hpp"
#include "ShellLib/ShellDropCopy.hpp"
#include "ShellLib/ShellEngine.hpp"
#include "ShellLib/ShellModule.hpp"
#include "WilliamCarryTimersDialog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::Strategies::WilliamCarry;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Shell;
using namespace trdk::FrontEnd::WilliamCarry;

namespace pt = boost::posix_time;
namespace ids = boost::uuids;

MultiBrokerWidget::MultiBrokerWidget(Engine &engine, QWidget *parent)
    : QWidget(parent),
      m_mode(TRADING_MODE_LIVE),
      m_engine(engine),
      m_currentSecurity(nullptr),
      m_ignoreLockToggling(false) {
  m_ui.setupUi(this);

  setEnabled(false);

  connect(m_ui.lock, &QPushButton::toggled, this,
          &MultiBrokerWidget::LockSecurity);

  connect(m_ui.buy1, &QPushButton::clicked,
          [this]() { OpenPosition(0, true); });
  connect(m_ui.sell1, &QPushButton::clicked,
          [this]() { OpenPosition(0, false); });
  connect(m_ui.buy2, &QPushButton::clicked,
          [this]() { OpenPosition(1, true); });
  connect(m_ui.sell2, &QPushButton::clicked,
          [this]() { OpenPosition(1, false); });
  connect(m_ui.buy3, &QPushButton::clicked,
          [this]() { OpenPosition(2, true); });
  connect(m_ui.sell3, &QPushButton::clicked,
          [this]() { OpenPosition(2, false); });
  connect(m_ui.buy4, &QPushButton::clicked,
          [this]() { OpenPosition(3, true); });
  connect(m_ui.sell4, &QPushButton::clicked,
          [this]() { OpenPosition(3, false); });
  connect(m_ui.closeAll, &QPushButton::clicked,
          [this]() { CloseAllPositions(); });

  connect(m_ui.showTimers, &QPushButton::clicked, this,
          &MultiBrokerWidget::ShowTimersSetupDialog);

  connect(m_ui.targetList, &QListWidget::currentRowChanged, [this](int index) {
    if (index < 0) {
      m_ui.securityList->setCurrentRow(-1);
      return;
    }
    ReloadSecurityList();
  });
  connect(m_ui.securityList, &QListWidget::currentRowChanged,
          [this](int index) {
            if (index < 0) {
              SetCurrentSecurity(nullptr);
              return;
            }
            AssertGt(m_securityList.size(), index);
            SetCurrentSecurity(&*m_securityList[index]);
          });

  connect(&m_engine, &Engine::StateChanged, this,
          &MultiBrokerWidget::OnStateChanged, Qt::QueuedConnection);

  connect(&m_engine.GetDropCopy(), &Shell::DropCopy::PriceUpdate, this,
          &MultiBrokerWidget::UpdatePrices, Qt::QueuedConnection);
}

void MultiBrokerWidget::OpenPosition(size_t strategyIndex, bool isLong) {
  AssertGt(m_strategies.size(), strategyIndex);
  Assert(m_strategies[strategyIndex]);

  const auto &delayMeasurement =
      m_engine.GetContext().StartStrategyTimeMeasurement();

  if (!m_currentSecurity) {
    QMessageBox::warning(
        this, tr("Failed to open new position"),
        QString("Please choose a currency to open new position."),
        QMessageBox::Ok);
    return;
  }

  for (;;) {
    OperationContext operation(isLong, 1);
    try {
      m_strategies[strategyIndex]->Invoke<Multibroker>(
          [this, &operation, &delayMeasurement](Multibroker &strategy) {
            strategy.OpenPosition(std::move(operation), *m_currentSecurity,
                                  delayMeasurement);
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
    if (m_currentSecurity) {
      m_lockedSecurityList.erase(m_currentSecurity);
    }
    ResetLockedPrices(m_currentSecurity);
    return;
  }

  if (!m_currentSecurity) {
    QMessageBox::warning(this, tr("Failed to send order"),
                         QString("Please choose a currency to send an order."),
                         QMessageBox::Ok);
    return;
  }

  const auto &locked = m_lockedSecurityList.emplace(std::make_pair(
      m_currentSecurity, Locked{m_currentSecurity->GetLastMarketDataTime(),
                                m_currentSecurity->GetBidPriceValue(),
                                m_currentSecurity->GetAskPriceValue()}));
  Assert(locked.second);
  SetLockedPrices(locked.first->second, *m_currentSecurity);
}

TradingSystem *MultiBrokerWidget::GetSelectedTradingSystem() {
  const auto &index = m_ui.targetList->currentRow();
  if (index < 0) {
    return nullptr;
  }
  return &m_engine.GetContext().GetTradingSystem(index, m_mode);
}

void MultiBrokerWidget::Reload() {
  m_currentSecurity = nullptr;
  m_securityList.clear();
  m_ui.securityList->clear();
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
  m_ui.securityList->clear();
  const auto *const tradingSystem = GetSelectedTradingSystem();
  if (!tradingSystem) {
    SetCurrentSecurity(nullptr);
    return;
  }
  Security *currentSecurity = nullptr;
  for (size_t i = 0; i < m_engine.GetContext().GetNumberOfMarketDataSources();
       ++i) {
    auto &source = m_engine.GetContext().GetMarketDataSource(i);
    if (source.GetInstanceName() != tradingSystem->GetInstanceName()) {
      continue;
    }
    source.ForEachSecurity([&](Security &security) {
      m_securityList.emplace_back(&security);
      m_ui.securityList->addItem(
#ifdef _DEBUG
          QString("%1 (%2)").arg(
              QString::fromStdString(security.GetSymbol().GetSymbol()),
              QString::fromStdString(security.GetSource().GetInstanceName()))
#else
          QString::fromStdString(security.GetSymbol().GetSymbol())
#endif
              );
      if (!currentSecurity) {
        currentSecurity = &security;
      }
    });
    break;
  }
  SetCurrentSecurity(currentSecurity);
}

void MultiBrokerWidget::SetCurrentSecurity(Security *security) {
  m_currentSecurity = security;
  if (!m_currentSecurity) {
    return;
  }
  m_ui.symbol->setText(
      QString::fromStdString(m_currentSecurity->GetSymbol().GetSymbol()));
  UpdatePrices(m_currentSecurity);
  {
    const auto &locked = m_lockedSecurityList.find(m_currentSecurity);
    Assert(!m_ignoreLockToggling);
    m_ignoreLockToggling = true;
    if (locked == m_lockedSecurityList.cend()) {
      ResetLockedPrices(m_currentSecurity);
      m_ui.lock->setChecked(false);
    } else {
      SetLockedPrices(locked->second, *m_currentSecurity);
      m_ui.lock->setChecked(true);
    }
    Assert(m_ignoreLockToggling);
    m_ignoreLockToggling = false;
  }
}

void MultiBrokerWidget::OnStateChanged(bool isStarted) {
  if (!isEnabled() && isStarted) {
    Reload();
  }
  setEnabled(isStarted);
}

void MultiBrokerWidget::UpdatePrices(const Security *security) {
  if (m_currentSecurity != security) {
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

////////////////////////////////////////////////////////////////////////////////

ModuleFactoryResult CreateEngineFrontEndWidgets(Engine &engine,
                                                QWidget *parent) {
  ModuleFactoryResult result;
  result.emplace_back("Multi Broker",
                      boost::make_unique<MultiBrokerWidget>(engine, parent));
  return result;
}

////////////////////////////////////////////////////////////////////////////////