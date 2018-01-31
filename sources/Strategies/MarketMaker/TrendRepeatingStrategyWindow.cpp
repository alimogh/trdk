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
    : Base(parent), m_engine(engine) /*, m_strategy(CreateStrategy(symbol))*/ {
  m_ui.setupUi(this);
  setWindowTitle(symbol + " " + tr("Market Making by Trend Strategy") + " - " +
                 QCoreApplication::applicationName());
  m_ui.symbol->setText(symbol);
  ConnectSignals();
}

TrendRepeatingStrategyWindow::~TrendRepeatingStrategyWindow() {}

void TrendRepeatingStrategyWindow::closeEvent(QCloseEvent *) {}

void TrendRepeatingStrategyWindow::OnBlocked(const QString &reason) {
  {
    const QSignalBlocker blocker(*m_ui.isPositionsOpeningEnabled);
    m_ui.isPositionsOpeningEnabled->setChecked(false);
  }
  {
    const QSignalBlocker blocker(*m_ui.isPositionsClosingEnabled);
    m_ui.isPositionsClosingEnabled->setChecked(false);
  }
  {
    m_ui.controlGroup->setEnabled(false);
    m_ui.positionGroup->setEnabled(false);
    m_ui.trendDetectionGroup->setEnabled(false);
  }
  ShowBlockedStrategyMessage(reason, this);
}

void TrendRepeatingStrategyWindow::ConnectSignals() {
  Verify(connect(m_ui.isPositionsOpeningEnabled, &QCheckBox::toggled, this,
                 &TrendRepeatingStrategyWindow::EnablePositionsOpening));
  Verify(connect(m_ui.isPositionsClosingEnabled, &QCheckBox::toggled, this,
                 &TrendRepeatingStrategyWindow::EnablePositionsClosing));

  Verify(connect(this, &TrendRepeatingStrategyWindow::Blocked, this,
                 &TrendRepeatingStrategyWindow::OnBlocked,
                 Qt::QueuedConnection));
}

void TrendRepeatingStrategyWindow::EnablePositionsOpening(bool isEnabed) {
  m_ui.isPositionsClosingEnabled->setEnabled(!isEnabed);
  if (isEnabed) {
    m_ui.isPositionsClosingEnabled->setChecked(true);
  }
}

void TrendRepeatingStrategyWindow::EnablePositionsClosing(bool isEnabed) {
  isEnabed;
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
    os << "[Strategy.TrendRepeatingMarketMaker/" << symbol.toStdString() << '/'
       << ++instanceNumber << "]" << std::endl
       << "module = MarketMaker" << std::endl
       << "id = " << strategyId << std::endl
       << "is_enabled = true" << std::endl
       << "trading_mode = live" << std::endl
       << "title = " << symbol.toStdString() << " Market Making by Trend"
       << std::endl
       << "requires = Level 1 Updates[" << symbol.toStdString() << "]"
       << std::endl;
    m_engine.GetContext().Add(IniString(os.str()));
  }
  return *boost::polymorphic_downcast<TrendRepeatingStrategy *>(
      &m_engine.GetContext().GetSrategy(strategyId));
}
