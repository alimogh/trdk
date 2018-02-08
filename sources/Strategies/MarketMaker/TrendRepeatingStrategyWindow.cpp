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
#include "TrendRepeatingStrategyWindow.hpp"
#include "TrendRepeatingStrategy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::Strategies::MarketMaker;

TrendRepeatingStrategyWindow::TrendRepeatingStrategyWindow(
    Engine &engine, const QString &symbol, QWidget *parent)
    : Base(parent), m_engine(engine), m_strategy(CreateStrategy(symbol)) {
  m_ui.setupUi(this);

  setWindowTitle(symbol + " " + tr("Market Making by Trend Strategy") + " - " +
                 QCoreApplication::applicationName());

  m_ui.symbol->setText(symbol);

  m_hasExchanges = LoadExchanges();
  if (!m_hasExchanges) {
    Disable();
    return;
  }

  m_ui.isPositionsOpeningEnabled->setChecked(m_strategy.IsTradingEnabled());
  m_ui.isPositionsClosingEnabled->setChecked(
      m_strategy.IsActivePositionsControlEnabled());

  m_ui.positionSize->setValue(m_strategy.GetPositionSize());

  m_ui.takeProfit->setValue(m_strategy.GetTakeProfit() * 100);
  m_ui.takeProfitTrailling->setValue(m_strategy.GetTakeProfitTrailing() * 100);
  m_ui.stopLoss->setValue(m_strategy.GetStopLoss() * 100);

  m_ui.fastMaPeriods->setValue(
      static_cast<int>(m_strategy.GetNumberOfFastMaPeriods()));
  m_ui.slowMaPeriods->setValue(
      static_cast<int>(m_strategy.GetNumberOfSlowMaPeriods()));

  ConnectSignals();
}

TrendRepeatingStrategyWindow::~TrendRepeatingStrategyWindow() {
  try {
    if (!m_strategy.IsBlocked()) {
      m_strategy.EnableTrading(false);
      m_strategy.EnableActivePositionsControl(false);
    }
  } catch (...) {
    AssertFailNoException();
    terminate();
  }
}

bool TrendRepeatingStrategyWindow::LoadExchanges() {
  auto &leftBox = *new QVBoxLayout(this);
  leftBox.setAlignment(Qt::AlignTop);
  auto &rightBox = *new QVBoxLayout(this);
  rightBox.setAlignment(Qt::AlignTop);

  size_t numberOfExchanges = 0;
  const auto &context = m_engine.GetContext();
  for (size_t i = 0; i < context.GetNumberOfTradingSystems(); ++i) {
    const boost::tribool &isEnabled = m_strategy.IsTradingSystemEnabled(i);
    auto *const checkBox = new QCheckBox(
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
                       const boost::tribool &isActuallyEnabled =
                           m_strategy.IsTradingSystemEnabled(i);
                       Assert(!boost::indeterminate(isActuallyEnabled));
                       checkBox->setChecked(isActuallyEnabled == true);
                     }
                   }));
    (i % 2 ? rightBox : leftBox).addWidget(checkBox);
  }

  auto &exchangesBox = *new QHBoxLayout(this);
  exchangesBox.addLayout(&leftBox);
  exchangesBox.addLayout(&rightBox);
  if (numberOfExchanges) {
    m_ui.exchangesGroup->setLayout(&exchangesBox);
  } else {
    auto &label = *new QLabel(this);
    label.setText(tr("There are no exchanges that support symbol %1!")
                      .arg(m_ui.symbol->text()));
    label.setAlignment(Qt::AlignCenter);
    auto &box = *new QVBoxLayout(this);
    box.addWidget(&label);
    box.addLayout(&exchangesBox);
    m_ui.exchangesGroup->setLayout(&box);
  }

  return numberOfExchanges > 0;
}

void TrendRepeatingStrategyWindow::Disable() {
  {
    const QSignalBlocker blocker(*m_ui.isPositionsOpeningEnabled);
    m_ui.isPositionsOpeningEnabled->setChecked(false);
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

void TrendRepeatingStrategyWindow::OnBlocked(const QString &reason) {
  Disable();
  ShowBlockedStrategyMessage(reason, this);
}

void TrendRepeatingStrategyWindow::OnStrategyEvent(const QString &message) {
  m_ui.log->appendPlainText(QString("%1: %2").arg(
      QDateTime::currentDateTime().time().toString(), message));
}

void TrendRepeatingStrategyWindow::ConnectSignals() {
  Verify(
      connect(m_ui.isPositionsOpeningEnabled, &QCheckBox::toggled,
              [this](bool isEnabled) {
                m_strategy.EnableTrading(isEnabled);
                {
                  const QSignalBlocker blocker(m_ui.isPositionsOpeningEnabled);
                  m_ui.isPositionsOpeningEnabled->setChecked(
                      m_strategy.IsTradingEnabled());
                }
                {
                  const QSignalBlocker blocker(m_ui.isPositionsClosingEnabled);
                  m_ui.isPositionsClosingEnabled->setChecked(
                      m_strategy.IsActivePositionsControlEnabled());
                }
                m_ui.isPositionsClosingEnabled->setEnabled(
                    !m_ui.isPositionsOpeningEnabled->isChecked());
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
                 }));

  Verify(connect(m_ui.fastMaPeriods,
                 static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                 [this](int value) {
                   m_strategy.SetNumberOfFastMaPeriods(value);
                   {
                     const QSignalBlocker blocker(*m_ui.fastMaPeriods);
                     m_ui.fastMaPeriods->setValue(static_cast<int>(
                         m_strategy.GetNumberOfFastMaPeriods()));
                   }
                 }));
  Verify(connect(m_ui.slowMaPeriods,
                 static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                 [this](int value) {
                   m_strategy.SetNumberOfSlowMaPeriods(value);
                   {
                     const QSignalBlocker blocker(*m_ui.slowMaPeriods);
                     m_ui.slowMaPeriods->setValue(static_cast<int>(
                         m_strategy.GetNumberOfSlowMaPeriods()));
                   }
                 }));

  Verify(connect(this, &TrendRepeatingStrategyWindow::Blocked, this,
                 &TrendRepeatingStrategyWindow::OnBlocked,
                 Qt::QueuedConnection));
  Verify(connect(this, &TrendRepeatingStrategyWindow::StrategyEvent, this,
                 &TrendRepeatingStrategyWindow::OnStrategyEvent,
                 Qt::QueuedConnection));
}

TrendRepeatingStrategy &TrendRepeatingStrategyWindow::CreateStrategy(
    const QString &symbol) {
  static boost::uuids::random_generator generateStrategyId;
  const auto &strategyId = generateStrategyId();
  {
    static size_t instanceNumber = 0;
    const IniFile conf(m_engine.GetConfigFilePath());
    const IniSectionRef defaults(conf, "Defaults");
    std::ostringstream os;
    os << "[Strategy.PingPong/" << symbol.toStdString() << '/'
       << ++instanceNumber << "]" << std::endl
       << "module = MarketMaker" << std::endl
       << "factory = CreateMaTrendRepeatingStrategy" << std::endl
       << "id = " << strategyId << std::endl
       << "is_enabled = true" << std::endl
       << "trading_mode = live" << std::endl
       << "title = " << symbol.toStdString() << " Ping Pong" << std::endl
       << "requires = Level 1 Updates[" << symbol.toStdString() << "]"
       << std::endl;
    m_engine.GetContext().Add(IniString(os.str()));
  }

  auto &result = *boost::polymorphic_downcast<TrendRepeatingStrategy *>(
      &m_engine.GetContext().GetSrategy(strategyId));

  result.Invoke<TrendRepeatingStrategy>([this](
                                            TrendRepeatingStrategy &strategy) {
    m_blockConnection =
        strategy.SubscribeToBlocking([this](const std::string *reasonSource) {
          QString reason;
          if (reasonSource) {
            reason = QString::fromStdString(*reasonSource);
          }
          emit Blocked(reason);
        });
    m_eventsConnection =
        strategy.SubscribeToEvents([this](const std::string &message) {
          emit StrategyEvent(QString::fromStdString(message));
        });
  });

  return result;
}
