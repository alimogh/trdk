/*******************************************************************************
 *   Created: 2018/01/31 21:37:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "StrategyWindow.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;
using namespace Strategies::PingPong;

namespace pp = Strategies::PingPong;
namespace pt = boost::posix_time;
namespace ids = boost::uuids;

StrategyWindow::StrategyWindow(Engine& engine,
                               const QString& symbol,
                               QWidget* parent)
    : Base(parent),
      m_engine(engine),
      m_symbol(symbol.toStdString()),
      m_strategy(GenerateNewStrategyInstance()) {
  Init();
  StoreConfig(true);
}

StrategyWindow::StrategyWindow(Engine& engine,
                               const QUuid& strategyId,
                               const QString& name,
                               const QString& config,
                               QWidget* parent)
    : Base(parent),
      m_engine(engine),
      m_symbol(ExtractSymbolFromConfig(config)),
      m_strategy(RestoreStrategyInstance(strategyId, name, config)) {
  Init();
}

StrategyWindow::~StrategyWindow() { m_strategy.Stop(); }

void StrategyWindow::Init() {
  setAttribute(Qt::WA_DeleteOnClose);

  m_ui.setupUi(this);

  setWindowTitle(QString::fromStdString(m_symbol) + " " +
                 tr("Ping Pong Strategy") + " - " +
                 QCoreApplication::applicationName());

  m_ui.symbol->setText(QString::fromStdString(m_symbol));

  m_hasExchanges = LoadExchanges();
  if (!m_hasExchanges) {
    Disable();
    return;
  }

  m_ui.isPositionsLongOpeningEnabled->setChecked(
      m_strategy.IsLongTradingEnabled());
  m_ui.isPositionsShortOpeningEnabled->setChecked(
      m_strategy.IsShortTradingEnabled());
  m_ui.isPositionsClosingEnabled->setChecked(
      m_strategy.IsActivePositionsControlEnabled());

  m_ui.positionSize->setValue(m_strategy.GetPositionSize());

  m_ui.timeFrameSize->setCurrentText(QString::number(
      m_strategy.GetSourceTimeFrameSize().total_seconds() / 60));

  m_ui.takeProfit->setValue(m_strategy.GetTakeProfit() * 100);
  m_ui.takeProfitTrailling->setValue(m_strategy.GetTakeProfitTrailing() * 100);
  m_ui.stopLoss->setValue(m_strategy.GetStopLoss() * 100);

  m_ui.isMaOpeningSignalConfirmationEnabled->setChecked(
      m_strategy.IsMaOpeningSignalConfirmationEnabled());
  m_ui.isMaClosingSignalConfirmationEnabled->setChecked(
      m_strategy.IsMaClosingSignalConfirmationEnabled());
  m_ui.fastMaPeriods->setValue(
      static_cast<int>(m_strategy.GetNumberOfFastMaPeriods()));
  m_ui.slowMaPeriods->setValue(
      static_cast<int>(m_strategy.GetNumberOfSlowMaPeriods()));

  m_ui.isRsiOpeningSignalConfirmationEnabled->setChecked(
      m_strategy.IsRsiOpeningSignalConfirmationEnabled());
  m_ui.isRsiClosingSignalConfirmationEnabled->setChecked(
      m_strategy.IsRsiClosingSignalConfirmationEnabled());
  m_ui.rsiPeriods->setValue(
      static_cast<int>(m_strategy.GetNumberOfRsiPeriods()));
  m_ui.rsiOverbought->setValue(
      static_cast<int>(m_strategy.GetRsiOverboughtLevel()));
  m_ui.rsiOversold->setValue(
      static_cast<int>(m_strategy.GetRsiOversoldLevel()));

  m_ui.bbPeriods->setValue(20);
  m_ui.bbDeviation->setValue(2);

  ConnectSignals();

  m_strategy.SetSourceTimeFrameSize(pt::minutes(5));
}

void StrategyWindow::closeEvent(QCloseEvent*) { StoreConfig(false); }

bool StrategyWindow::LoadExchanges() {
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
            context.GetTradingSystem(i, TRADING_MODE_LIVE).GetInstanceName()),
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

void StrategyWindow::Disable() {
  {
    const QSignalBlocker blocker(*m_ui.isPositionsLongOpeningEnabled);
    m_ui.isPositionsLongOpeningEnabled->setChecked(false);
  }
  {
    const QSignalBlocker blocker(*m_ui.isPositionsShortOpeningEnabled);
    m_ui.isPositionsShortOpeningEnabled->setChecked(false);
  }
  {
    const QSignalBlocker blocker(*m_ui.isPositionsClosingEnabled);
    m_ui.isPositionsClosingEnabled->setChecked(false);
  }
  {
    if (m_hasExchanges) {
      m_ui.exchangesGroup->setEnabled(false);
    }
    m_ui.controlGroup->setEnabled(false);
    m_ui.positionGroup->setEnabled(false);
    m_ui.trendDetectionGroup->setEnabled(false);
  }
}

void StrategyWindow::OnBlocked(const QString& reason) {
  Disable();
  ShowBlockedStrategyMessage(reason, this);
}

void StrategyWindow::OnStrategyEvent(const QString& message) {
  m_ui.log->appendPlainText(QString("%1: %2").arg(
      QDateTime::currentDateTime().time().toString(), message));
}

void StrategyWindow::ConnectSignals() {
  m_blockConnection =
      m_strategy.SubscribeToBlocking([this](const std::string* reasonSource) {
        QString reason;
        if (reasonSource) {
          reason = QString::fromStdString(*reasonSource);
        }
        emit Blocked(reason);
      });
  m_eventsConnection =
      m_strategy.SubscribeToEvents([this](const std::string& message) {
        emit StrategyEvent(QString::fromStdString(message));
      });

  Verify(connect(
      m_ui.isPositionsLongOpeningEnabled, &QCheckBox::toggled,
      [this](bool isEnabled) {
        m_strategy.EnableLongTrading(isEnabled);
        {
          const QSignalBlocker blocker(m_ui.isPositionsLongOpeningEnabled);
          m_ui.isPositionsLongOpeningEnabled->setChecked(
              m_strategy.IsLongTradingEnabled());
        }
        {
          const QSignalBlocker blocker(m_ui.isPositionsClosingEnabled);
          m_ui.isPositionsClosingEnabled->setChecked(
              m_strategy.IsActivePositionsControlEnabled());
        }
      }));
  Verify(connect(
      m_ui.isPositionsShortOpeningEnabled, &QCheckBox::toggled,
      [this](bool isEnabled) {
        m_strategy.EnableShortTrading(isEnabled);
        {
          const QSignalBlocker blocker(m_ui.isPositionsShortOpeningEnabled);
          m_ui.isPositionsShortOpeningEnabled->setChecked(
              m_strategy.IsShortTradingEnabled());
        }
        {
          const QSignalBlocker blocker(m_ui.isPositionsClosingEnabled);
          m_ui.isPositionsClosingEnabled->setChecked(
              m_strategy.IsActivePositionsControlEnabled());
        }
      }));
  Verify(connect(m_ui.isPositionsClosingEnabled, &QCheckBox::toggled,
                 [this](bool isEnabled) {
                   m_strategy.EnableActivePositionsControl(isEnabled);
                   {
                     const QSignalBlocker blocker(
                         m_ui.isPositionsClosingEnabled);
                     m_ui.isPositionsClosingEnabled->setChecked(
                         m_strategy.IsActivePositionsControlEnabled());
                   }
                   StoreConfig(true);
                 }));

  Verify(connect(m_ui.timeFrameSize, &QComboBox::currentTextChanged,
                 [this](const QString& item) {
                   m_strategy.SetSourceTimeFrameSize(pt::minutes(item.toInt()));
                   StoreConfig(true);
                 }));

  Verify(connect(m_ui.positionSize,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double value) {
                   m_strategy.SetPositionSize(value);
                   {
                     const QSignalBlocker blocker(*m_ui.positionSize);
                     m_ui.positionSize->setValue(m_strategy.GetPositionSize());
                   }
                   StoreConfig(true);
                 }));
  Verify(connect(m_ui.takeProfit,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double value) {
                   m_strategy.SetTakeProfit(value / 100);
                   {
                     const QSignalBlocker blocker(*m_ui.takeProfit);
                     m_ui.takeProfit->setValue(m_strategy.GetTakeProfit() *
                                               100);
                   }
                   StoreConfig(true);
                 }));
  Verify(connect(m_ui.takeProfitTrailling,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double value) {
                   m_strategy.SetTakeProfitTrailing(value / 100);
                   {
                     const QSignalBlocker blocker(*m_ui.takeProfitTrailling);
                     m_ui.takeProfitTrailling->setValue(
                         m_strategy.GetTakeProfitTrailing() * 100);
                   }
                   StoreConfig(true);
                 }));
  Verify(connect(m_ui.stopLoss,
                 static_cast<void (QDoubleSpinBox::*)(double)>(
                     &QDoubleSpinBox::valueChanged),
                 [this](double value) {
                   m_strategy.SetStopLoss(value / 100);
                   {
                     const QSignalBlocker blocker(*m_ui.stopLoss);
                     m_ui.stopLoss->setValue(m_strategy.GetStopLoss() * 100);
                   }
                   StoreConfig(true);
                 }));

  {
    Verify(connect(m_ui.isMaOpeningSignalConfirmationEnabled,
                   &QCheckBox::toggled, [this](bool isEnabled) {
                     m_strategy.EnableMaOpeningSignalConfirmation(isEnabled);
                     {
                       const QSignalBlocker blocker(
                           m_ui.isMaOpeningSignalConfirmationEnabled);
                       m_ui.isMaOpeningSignalConfirmationEnabled->setChecked(
                           m_strategy.IsMaOpeningSignalConfirmationEnabled());
                     }
                     StoreConfig(true);
                   }));
    Verify(connect(m_ui.isMaClosingSignalConfirmationEnabled,
                   &QCheckBox::toggled, [this](bool isEnabled) {
                     m_strategy.EnableMaClosingSignalConfirmation(isEnabled);
                     {
                       const QSignalBlocker blocker(
                           m_ui.isMaClosingSignalConfirmationEnabled);
                       m_ui.isMaClosingSignalConfirmationEnabled->setChecked(
                           m_strategy.IsMaClosingSignalConfirmationEnabled());
                     }
                     StoreConfig(true);
                   }));
    Verify(connect(
        m_ui.fastMaPeriods,
        static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
        [this](int value) {
          m_strategy.SetNumberOfFastMaPeriods(value);
          {
            const QSignalBlocker blocker(*m_ui.fastMaPeriods);
            m_ui.fastMaPeriods->setValue(
                static_cast<int>(m_strategy.GetNumberOfFastMaPeriods()));
          }
          StoreConfig(true);
        }));
    Verify(connect(
        m_ui.slowMaPeriods,
        static_cast<void (QSpinBox ::*)(int)>(&QSpinBox::valueChanged),
        [this](int value) {
          m_strategy.SetNumberOfSlowMaPeriods(value);
          {
            const QSignalBlocker blocker(*m_ui.slowMaPeriods);
            m_ui.slowMaPeriods->setValue(
                static_cast<int>(m_strategy.GetNumberOfSlowMaPeriods()));
          }
          StoreConfig(true);
        }));
  }

  {
    Verify(connect(m_ui.isRsiOpeningSignalConfirmationEnabled,
                   &QCheckBox::toggled, [this](bool isEnabled) {
                     m_strategy.EnableRsiOpeningSignalConfirmation(isEnabled);
                     {
                       const QSignalBlocker blocker(
                           m_ui.isRsiOpeningSignalConfirmationEnabled);
                       m_ui.isRsiOpeningSignalConfirmationEnabled->setChecked(
                           m_strategy.IsRsiOpeningSignalConfirmationEnabled());
                     }
                     StoreConfig(true);
                   }));
    Verify(connect(m_ui.isRsiClosingSignalConfirmationEnabled,
                   &QCheckBox::toggled, [this](bool isEnabled) {
                     m_strategy.EnableRsiClosingSignalConfirmation(isEnabled);
                     {
                       const QSignalBlocker blocker(
                           m_ui.isRsiClosingSignalConfirmationEnabled);
                       m_ui.isRsiClosingSignalConfirmationEnabled->setChecked(
                           m_strategy.IsRsiClosingSignalConfirmationEnabled());
                     }
                     StoreConfig(true);
                   }));
    Verify(connect(m_ui.rsiOverbought,
                   static_cast<void (QDoubleSpinBox ::*)(double)>(
                       &QDoubleSpinBox ::valueChanged),
                   [this](double value) {
                     m_strategy.SetRsiOverboughtLevel(value);
                     {
                       const QSignalBlocker blocker(*m_ui.rsiOverbought);
                       m_ui.rsiOverbought->setValue(
                           m_strategy.GetRsiOverboughtLevel());
                     }
                     StoreConfig(true);
                   }));
    Verify(connect(m_ui.rsiOversold,
                   static_cast<void (QDoubleSpinBox ::*)(double)>(
                       &QDoubleSpinBox ::valueChanged),
                   [this](double value) {
                     m_strategy.SetRsiOversoldLevel(value);
                     {
                       const QSignalBlocker blocker(*m_ui.rsiOversold);
                       m_ui.rsiOversold->setValue(
                           m_strategy.GetRsiOversoldLevel());
                     }
                     StoreConfig(true);
                   }));
  }

  Verify(connect(this, &StrategyWindow ::Blocked, this,
                 &StrategyWindow ::OnBlocked, Qt ::QueuedConnection));
  Verify(connect(this, &StrategyWindow ::StrategyEvent, this,
                 &StrategyWindow ::OnStrategyEvent, Qt ::QueuedConnection));
}

pp::Strategy& StrategyWindow ::CreateStrategyInstance(
    const ids ::uuid& strategyId,
    const QString& name,
    const std ::string& config) {
  std ::ostringstream ini;
  ini << "[Strategy." << name << "]" << std::endl << config;
  m_engine.GetContext().Add(IniString(ini.str()));
  return *boost ::polymorphic_downcast<Strategy*>(
      &m_engine.GetContext().GetSrategy(strategyId));
}

pp::Strategy& StrategyWindow ::GenerateNewStrategyInstance() {
  static ids::random_generator generateStrategyId;
  const auto& id = generateStrategyId();
  return CreateStrategyInstance(
      id,
      m_engine.GenerateNewStrategyName(
          QString::fromStdString("Ping Pong " + m_symbol)),
      CreateConfig(id, false, false, true, .01, false, false, 12, 26, false,
                   false, 14, 70, 30, 3, .75, 15, pt ::minutes(5)));
}

pp ::Strategy& StrategyWindow ::RestoreStrategyInstance(const QUuid& strategyId,
                                                        const QString& name,
                                                        const QString& config) {
  return CreateStrategyInstance(ConvertToBoostUuid(strategyId), name,
                                config.toStdString());
}

std ::string StrategyWindow ::CreateConfig(
    const ids ::uuid& strategyId,
    const bool isLongOpeningEnabled,
    const bool isShortOpeningEnabled,
    const bool isActivePositionsControlEnabled,
    const Qty& positionSize,
    const bool isMaOpeningSignalConfirmationEnabled,
    const bool isMaClosingSignalConfirmationEnabled,
    const size_t fastMaSize,
    const size_t slowMaSize,
    const bool isRsiOpeningSignalConfirmationEnabled,
    const bool isRsiClosingSignalConfirmationEnabled,
    const size_t numberOfRsiPeriods,
    const Double& rsiOverboughtLevel,
    const Double& rsiOversoldLevel,
    const Volume& profitShareToActivateTakeProfit,
    const Volume& takeProfitTrailingShareToClose,
    const Double& maxLossShare,
    const pt ::time_duration& frameSize) const {
  std ::ostringstream result;
  result << "module = PingPong" << std ::endl;
  result << "factory = CreateStrategy" << std ::endl;
  result << "id = " << strategyId << std ::endl;
  result << "is_enabled = true" << std ::endl;
  result << "trading_mode = live" << std ::endl;
  result << "requires = Level 1 Updates[" << m_symbol << "]" << std ::endl;
  result << "symbol = " << m_symbol << std ::endl;

  result << "long_trading_enabled = " << (isLongOpeningEnabled ? "yes" : "no")
         << std ::endl;
  result << "short_trading_enabled = " << (isShortOpeningEnabled ? "yes" : "no")
         << std ::endl;

  result << "source_time_frame_size_sec = " << frameSize.total_seconds()
         << std ::endl;

  result << "active_positions_control_enabled = "
         << (isActivePositionsControlEnabled ? "yes" : "no") << std ::endl;

  result << "ma_opening_signal_confirmation_enabled = "
         << (isMaOpeningSignalConfirmationEnabled ? "yes" : "no") << std ::endl;
  result << "ma_closing_signal_confirmation_enabled = "
         << (isMaClosingSignalConfirmationEnabled ? "yes" : "no") << std ::endl;
  result << "number_of_fast_ma_periods = " << fastMaSize << std ::endl;
  result << "number_of_slow_ma_periods = " << slowMaSize << std ::endl;

  result << "rsi_opening_signal_confirmation_enabled = "
         << (isRsiOpeningSignalConfirmationEnabled ? "yes" : "no") << std::endl;
  result << "rsi_closing_signal_confirmation_enabled = "
         << (isRsiClosingSignalConfirmationEnabled ? "yes" : "no") << std::endl;
  result << "number_of_rsi_periods = " << numberOfRsiPeriods << std ::endl;
  result << "rsi_overbought_level = " << rsiOverboughtLevel << std ::endl;
  result << "rsi_oversold_level = " << rsiOversoldLevel << std ::endl;

  result << "position_size = " << positionSize << std ::endl;

  result << "max_loss_share = " << maxLossShare << std ::endl;

  result << "profit_share_to_activate_take_profit = "
         << profitShareToActivateTakeProfit << std ::endl;
  result << "take_profit_trailing_share_to_close = "
         << takeProfitTrailingShareToClose << std ::endl;

  return result.str();
}

std ::string StrategyWindow ::DumpConfig() const {
  return CreateConfig(
      m_strategy.GetId(), m_ui.isPositionsLongOpeningEnabled->isChecked(),
      m_ui.isPositionsShortOpeningEnabled->isChecked(),
      m_ui.isPositionsClosingEnabled->isChecked(), m_ui.positionSize->value(),
      m_ui.isMaOpeningSignalConfirmationEnabled->isChecked(),
      m_ui.isMaClosingSignalConfirmationEnabled->isChecked(),
      m_ui.fastMaPeriods->value(), m_ui.slowMaPeriods->value(),
      m_ui.isRsiOpeningSignalConfirmationEnabled->isChecked(),
      m_ui.isRsiClosingSignalConfirmationEnabled->isChecked(),
      m_ui.rsiPeriods->value(), m_ui.rsiOverbought->value(),
      m_ui.rsiOversold->value(), m_ui.takeProfit->value(),
      m_ui.takeProfitTrailling->value(), m_ui.stopLoss->value(),
      pt ::minutes(m_ui.timeFrameSize->currentText().toInt()));
}

void StrategyWindow ::StoreConfig(bool isActive) {
  m_engine.StoreConfig(m_strategy, QString ::fromStdString(DumpConfig()),
                       isActive);
}
