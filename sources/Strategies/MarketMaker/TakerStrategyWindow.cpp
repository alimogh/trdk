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
using namespace trdk::Lib;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::Strategies::MarketMaker;

TakerStrategyWindow::TakerStrategyWindow(Engine &engine,
                                         const QString &symbol,
                                         QWidget *parent)
    : Base(parent),
      m_engine(engine),
      m_strategy(CreateStrategyInstance(symbol)) {
  setAttribute(Qt::WA_DeleteOnClose);

  m_ui.setupUi(this);

  setWindowTitle(symbol + " " + tr("Market Maker \"Taker\"") + " - " +
                 QCoreApplication::applicationName());

  m_ui.symbol->setText(symbol);
  {
    const auto &currencies = symbol.split('_', QString::SkipEmptyParts);
    AssertEq(2, currencies.size());
    const auto &baseCurrency =
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
  m_ui.periodSize->setValue(static_cast<int>(m_strategy.GetPeriodSize()));
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

void TakerStrategyWindow::Disable() {}

void TakerStrategyWindow::OnCompleted() {
  QMessageBox::information(this, QObject::tr("Strategy completed"),
                           QObject::tr("Strategy is completed."),
                           QMessageBox::Ok);
  m_ui.activationToggle->setChecked(m_strategy.IsTradingEnabled());
}

void TakerStrategyWindow::OnBlocked(const QString &reason) {
  Disable();
  ShowBlockedStrategyMessage(reason, this);
}

void TakerStrategyWindow::OnVolumeUpdate(const Volume &currentVolume,
                                         const Volume &maxVolume) {
  m_ui.volumeBar->setValue(static_cast<int>(currentVolume * 100000000));
  m_ui.volumeBar->setMaximum(static_cast<int>(maxVolume * 100000000));
}

void TakerStrategyWindow::OnPnlUpdate(const Volume &currentPnl,
                                      const Volume &maxLoss) {
  m_ui.lossBar->setValue(
      static_cast<int>(std::max(.0, -currentPnl.Get()) * 100000000));
  m_ui.lossBar->setMaximum(static_cast<int>(maxLoss * 100000000));
}

void TakerStrategyWindow::OnStrategyEvent(const QString &message) {
  m_ui.log->appendPlainText(QString("%1: %2").arg(
      QDateTime::currentDateTime().time().toString(), message));
}

void TakerStrategyWindow::ConnectSignals() {
  qRegisterMetaType<trdk::Volume>("trdk::Volume");
  qRegisterMetaType<trdk::Volume>("Volume");
  qRegisterMetaType<trdk::Qty>("trdk::Qty");
  qRegisterMetaType<trdk::Qty>("Qty");
  qRegisterMetaType<trdk::Price>("trdk::Price");
  qRegisterMetaType<trdk::Price>("Price");

  Verify(connect(
      m_ui.activationToggle, &QCheckBox::toggled, [this](bool isEnabled) {
        m_strategy.EnableTrading(isEnabled);
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
                   m_strategy.SetPeriodSize(value);
                   {
                     const QSignalBlocker blocker(*m_ui.periodSize);
                     m_ui.periodSize->setValue(
                         static_cast<int>(m_strategy.GetPeriodSize()));
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

TakerStrategy &TakerStrategyWindow::CreateStrategyInstance(
    const QString &symbol) {
  static boost::uuids::random_generator generateStrategyId;
  const auto &strategyId = generateStrategyId();
  {
    static size_t instanceNumber = 0;
    const IniFile conf(m_engine.GetConfigFilePath());
    const IniSectionRef defaults(conf, "Defaults");
    std::ostringstream os;
    os << "[Strategy.Taker/" << symbol.toStdString() << '/' << ++instanceNumber
       << "]" << std::endl
       << "module = MarketMaker" << std::endl
       << "factory = CreateStrategy" << std::endl
       << "id = " << strategyId << std::endl
       << "is_enabled = true" << std::endl
       << "trading_mode = live" << std::endl
       << "title = Taker" << std::endl
       << "requires = Level 1 Updates[" << symbol.toStdString() << "]"
       << std::endl;
    m_engine.GetContext().Add(IniString(os.str()));
  }
  auto &result = *boost::polymorphic_downcast<TakerStrategy *>(
      &m_engine.GetContext().GetSrategy(strategyId));

  result.Invoke<TakerStrategy>([this](TakerStrategy &strategy) {
    m_completedConnection =
        strategy.SubscribeToCompleted([this]() { emit Completed(); });
    m_blockConnection =
        strategy.SubscribeToBlocking([this](const std::string *reasonSource) {
          QString reason;
          if (reasonSource) {
            reason = QString::fromStdString(*reasonSource);
          }
          emit Blocked(reason);
        });
    m_volumeUpdateConnection = strategy.SubscribeToVolume(
        [this](const Volume &currentVolume, const Volume &maxVolume) {
          emit VolumeUpdate(currentVolume, maxVolume);
        });
    m_pnlUpdateConnection = strategy.SubscribeToPnl(
        [this](const Volume &currentPnl, const Volume &maxLoss) {
          emit PnlUpdate(currentPnl, maxLoss);
        });
    m_eventsConnection =
        strategy.SubscribeToEvents([this](const std::string &message) {
          emit StrategyEvent(QString::fromStdString(message));
        });
  });

  return result;
}
