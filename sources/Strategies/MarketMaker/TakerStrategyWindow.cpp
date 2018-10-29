/*******************************************************************************
 *   Created: 2018/02/20 22:48:17
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "TakerStrategyWindow.hpp"
#include "TakerStrategy.hpp"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;
using namespace Strategies::MarketMaker;
namespace pt = boost::posix_time;
namespace ptr = boost::property_tree;

TakerStrategyWindow::TakerStrategyWindow(
    Engine& engine,
    const QString& symbol,
    Context::AddingTransaction& transaction,
    QWidget* parent)
    : Base(parent),
      m_engine(engine),
      m_strategy(CreateStrategyInstance(symbol, transaction)) {
  m_ui.setupUi(this);

  setWindowTitle(symbol + " " + tr(R"(Market Maker "Taker")") + " - " +
                 QCoreApplication::applicationName());

  m_ui.symbol->setText(symbol);
  {
    const auto& currencies = symbol.split('_', QString::SkipEmptyParts);
    AssertEq(2, currencies.size());
    const auto& baseCurrency =
        currencies.size() == 2 ? currencies.back() : symbol;
    m_ui.maxLossVolumeSymbol->setText(baseCurrency);
    m_ui.volumeGoalSymbol->setText(baseCurrency);
    m_ui.minTradeVolumeSymbol->setText(baseCurrency);
    m_ui.maxTradeVolumeSymbol->setText(baseCurrency);
  }

  m_ui.activationToggle->setChecked(m_strategy.IsTradingEnabled());
  m_ui.stopAfterNumberOfPeriods->setValue(
      static_cast<int>(m_strategy.GetNumerOfPeriods()));
  m_ui.maxLossVolume->setValue(m_strategy.GetMaxLoss());
  m_ui.periodSize->setValue(
      static_cast<int>(m_strategy.GetPeriodSize().total_seconds() / 60));
  m_ui.goalVolume->setValue(m_strategy.GetGoalVolume());
  m_ui.priceRangeFrom->setValue(m_strategy.GetMinPrice());
  m_ui.priceRangeTo->setValue(m_strategy.GetMaxPrice());
  m_ui.minTradeVolume->setValue(m_strategy.GetTradeMinVolume());
  m_ui.maxTradeVolume->setValue(m_strategy.GetTradeMaxVolume());

  m_hasExchanges = LoadExchanges();
  if (!m_hasExchanges) {
    Disable();
    return;
  }

  ConnectSignals();
}

TakerStrategyWindow::~TakerStrategyWindow() { m_strategy.Stop(); }

bool TakerStrategyWindow::LoadExchanges() {
  auto& leftBox = *new QVBoxLayout(this);
  leftBox.setAlignment(Qt::AlignTop);
  auto& rightBox = *new QVBoxLayout(this);
  rightBox.setAlignment(Qt::AlignTop);

  size_t numberOfExchanges = 0;
  const auto& context = m_engine.GetContext();
  for (size_t i = 0; i < context.GetNumberOfTradingSystems(); ++i) {
    const boost::tribool& isEnabled = m_strategy.IsTradingSystemEnabled(i);
    auto* const checkBox = new QCheckBox(
        QString::fromStdString(
            context.GetTradingSystem(i, TRADING_MODE_LIVE).GetTitle()),
        this);
    if (!boost::indeterminate(isEnabled)) {
      ++numberOfExchanges;
      checkBox->setChecked(isEnabled);
    } else {
      checkBox->setEnabled(false);
    }
    Verify(connect(checkBox, &QCheckBox::toggled,
                   [this, i, checkBox](bool isEnabled) {
                     m_strategy.EnableTradingSystem(i, isEnabled);
                     {
                       const QSignalBlocker blocker(*checkBox);
                       const boost::tribool& isActuallyEnabled =
                           m_strategy.IsTradingSystemEnabled(i);
                       Assert(!boost::indeterminate(isActuallyEnabled));
                       checkBox->setChecked(isActuallyEnabled == true);
                     }
                   }));
    (i % 2 ? rightBox : leftBox).addWidget(checkBox);
  }

  auto& exchangesBox = *new QHBoxLayout(this);
  exchangesBox.addLayout(&leftBox);
  exchangesBox.addLayout(&rightBox);
  if (numberOfExchanges) {
    m_ui.exchangesGroup->setLayout(&exchangesBox);
  } else {
    auto& label = *new QLabel(this);
    label.setText(tr("There are no exchanges that support symbol %1!")
                      .arg(m_ui.symbol->text()));
    label.setAlignment(Qt::AlignCenter);
    auto& box = *new QVBoxLayout(this);
    box.addWidget(&label);
    box.addLayout(&exchangesBox);
    m_ui.exchangesGroup->setLayout(&box);
  }

  return numberOfExchanges > 0;
}

void TakerStrategyWindow::Disable() {
  const QSignalBlocker blocker(*m_ui.controlGroup);
  m_ui.controlGroup->setEnabled(false);
}

void TakerStrategyWindow::OnCompleted() {
  const QSignalBlocker lock(*m_ui.activationToggle);
  m_ui.activationToggle->setChecked(m_strategy.IsTradingEnabled());
}

void TakerStrategyWindow::OnBlocked(const QString& reason) {
  Disable();
  ShowBlockedStrategyMessage(reason, this);
}

namespace {
void SetProgressBarValue(QProgressBar& bar,
                         const Volume& current,
                         const Volume& max) {
  const auto currentScaled = static_cast<int>(current * 100000000);
  const auto maxScaled = static_cast<int>(max * 100000000);
  bar.setValue(std::min(currentScaled, maxScaled));
  bar.setMaximum(maxScaled);
}
}  // namespace
void TakerStrategyWindow::OnVolumeUpdate(const Volume& currentVolume,
                                         const Volume& maxVolume) {
  SetProgressBarValue(*m_ui.volumeBar, currentVolume, maxVolume);
}

void TakerStrategyWindow::OnPnlUpdate(const Volume& currentPnl,
                                      const Volume& maxLoss) {
  SetProgressBarValue(*m_ui.lossBar, currentPnl < 0 ? -currentPnl : 0, maxLoss);
}

void TakerStrategyWindow::OnStrategyEvent(const QString& message) {
  m_ui.log->appendPlainText(QString("%1: %2").arg(
      QDateTime::currentDateTime().time().toString(), message));
}

void TakerStrategyWindow::ConnectSignals() {
  qRegisterMetaType<Volume>("trdk::Volume");
  qRegisterMetaType<Volume>("Volume");
  qRegisterMetaType<Qty>("trdk::Qty");
  qRegisterMetaType<Qty>("Qty");
  qRegisterMetaType<Price>("trdk::Price");
  qRegisterMetaType<Price>("Price");

  Verify(connect(
      m_ui.activationToggle, &QCheckBox::toggled, [this](bool isEnabled) {
        try {
          m_strategy.EnableTrading(isEnabled);
        } catch (const std::exception& ex) {
          QMessageBox::critical(this, QObject::tr("Failed to enable trading"),
                                ex.what(), QMessageBox::Ok);
        }
        {
          const QSignalBlocker blocker(m_ui.activationToggle);
          m_ui.activationToggle->setChecked(m_strategy.IsTradingEnabled());
        }
      }));

  Verify(connect(m_ui.stopAfterNumberOfPeriods,
                 static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                 [this](int value) {
                   m_strategy.SetNumerOfPeriods(value);
                   {
                     const QSignalBlocker blocker(
                         *m_ui.stopAfterNumberOfPeriods);
                     m_ui.stopAfterNumberOfPeriods->setValue(
                         static_cast<int>(m_strategy.GetNumerOfPeriods()));
                   }
                 }));

  Verify(connect(m_ui.maxLossVolume,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double value) {
                   m_strategy.SetMaxLoss(value);
                   {
                     const QSignalBlocker blocker(*m_ui.maxLossVolume);
                     m_ui.maxLossVolume->setValue(m_strategy.GetMaxLoss());
                   }
                 }));

  Verify(connect(m_ui.periodSize,
                 static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                 [this](int value) {
                   m_strategy.SetPeriodSize(pt::minutes(value));
                   {
                     const QSignalBlocker blocker(*m_ui.periodSize);
                     m_ui.periodSize->setValue(static_cast<int>(
                         m_strategy.GetPeriodSize().total_seconds() / 60));
                   }
                 }));

  Verify(connect(m_ui.goalVolume,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double value) {
                   m_strategy.SetGoalVolume(value);
                   {
                     const QSignalBlocker blocker(*m_ui.goalVolume);
                     m_ui.goalVolume->setValue(m_strategy.GetGoalVolume());
                   }
                 }));

  Verify(connect(m_ui.priceRangeFrom,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double value) {
                   m_strategy.SetMinPrice(value);
                   {
                     const QSignalBlocker blocker(*m_ui.priceRangeFrom);
                     m_ui.priceRangeFrom->setValue(m_strategy.GetMinPrice());
                   }
                 }));
  Verify(connect(m_ui.priceRangeTo,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double value) {
                   m_strategy.SetMaxPrice(value);
                   {
                     const QSignalBlocker blocker(*m_ui.priceRangeTo);
                     m_ui.priceRangeTo->setValue(m_strategy.GetMaxPrice());
                   }
                 }));

  Verify(connect(m_ui.minTradeVolume,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double value) {
                   m_strategy.SetTradeMinVolume(value);
                   {
                     const QSignalBlocker blocker(*m_ui.minTradeVolume);
                     m_ui.minTradeVolume->setValue(
                         m_strategy.GetTradeMinVolume());
                   }
                 }));
  Verify(connect(m_ui.maxTradeVolume,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double value) {
                   m_strategy.SetTradeMaxVolume(value);
                   {
                     const QSignalBlocker blocker(*m_ui.maxTradeVolume);
                     m_ui.maxTradeVolume->setValue(
                         m_strategy.GetTradeMaxVolume());
                   }
                 }));

  Verify(connect(this, &TakerStrategyWindow::Completed, this,
                 &TakerStrategyWindow::OnCompleted, Qt::QueuedConnection));
  Verify(connect(this, &TakerStrategyWindow::Blocked, this,
                 &TakerStrategyWindow::OnBlocked, Qt::QueuedConnection));
  Verify(connect(this, &TakerStrategyWindow::VolumeUpdate, this,
                 &TakerStrategyWindow::OnVolumeUpdate, Qt::QueuedConnection));
  Verify(connect(this, &TakerStrategyWindow::PnlUpdate, this,
                 &TakerStrategyWindow::OnPnlUpdate, Qt::QueuedConnection));
  Verify(connect(this, &TakerStrategyWindow::StrategyEvent, this,
                 &TakerStrategyWindow::OnStrategyEvent, Qt::QueuedConnection));
}

TakerStrategy& TakerStrategyWindow ::CreateStrategyInstance(
    const QString& symbol, Context::AddingTransaction& transaction) {
  static boost::uuids::random_generator generateStrategyId;
  const auto& strategyId = generateStrategyId();
  {
    ptr::ptree config;
    config.add("module", "MarketMaker");
    config.add("id", strategyId);
    config.add("isEnabled", true);
    config.add("tradingMode", "live");
    {
      ptr::ptree symbols;
      symbols.push_back({"", ptr::ptree().put("", symbol)});
      config.add_child("requirements.level1Updates.symbols", symbols);
    }
    ptr::ptree strategiesConfig;
    strategiesConfig.add_child(
        m_engine.GenerateNewStrategyInstanceName(R"(Market Maker "Taker" )" +
                                                 symbol.toStdString()),
        config);
    transaction.Add(strategiesConfig);
  }
  auto& result = *boost::polymorphic_downcast<TakerStrategy*>(
      &transaction.GetStrategy(strategyId));

  m_completedConnection =
      result.SubscribeToCompleted([this]() { emit Completed(); });
  m_blockConnection =
      result.SubscribeToBlocking([this](const std ::string* reasonSource) {
        QString reason;
        if (reasonSource) {
          reason = QString ::fromStdString(*reasonSource);
        }
        emit Blocked(reason);
      });
  m_volumeUpdateConnection = result.SubscribeToVolume(
      [this](const Volume& currentVolume, const Volume& maxVolume) {
        emit VolumeUpdate(currentVolume, maxVolume);
      });
  m_pnlUpdateConnection = result.SubscribeToPnl(
      [this](const Volume& currentPnl, const Volume& maxLoss) {
        emit PnlUpdate(currentPnl, maxLoss);
      });
  m_eventsConnection =
      result.SubscribeToEvents([this](const std::string& message) {
        emit StrategyEvent(QString ::fromStdString(message));
      });

  return result;
}
