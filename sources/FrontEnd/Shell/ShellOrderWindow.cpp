/*******************************************************************************
 *   Created: 2017/09/30 04:41:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "ShellOrderWindow.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "ShellLib/ShellDropCopy.hpp"
#include "ShellLib/ShellEngine.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::FrontEnd::Shell;

namespace sh = trdk::FrontEnd::Shell;
namespace pt = boost::posix_time;

OrderWindow::OrderWindow(sh::Engine &engine,
                         Security &security,
                         QWidget *parent)
    : Base(parent), m_engine(engine), m_security(security) {
  m_ui.setupUi(this);

  const auto &symbol = QString::fromStdString(security.GetSymbol().GetSymbol());
  setWindowTitle(symbol + " - " + QCoreApplication::applicationName());
  m_ui.symbol->setText(symbol);
  m_ui.source->setText(
      QString::fromStdString(security.GetSource().GetInstanceName()));
  m_ui.fullSymbol->setText(
      QString::fromStdString(security.GetSymbol().GetAsString()));

  m_ui.qty->setDecimals(m_security.GetPricePrecision());

  if (m_engine.GetContext().GetNumberOfTradingSystems() > 0) {
    boost::optional<int> selectedByDefault;
    for (int i = 0; i < numberOfTradingModes; ++i) {
      try {
        m_engine.GetContext().GetTradingSystem(0, static_cast<TradingMode>(i));
      } catch (const Context::TrtadingModeIsNotLoad &) {
        continue;
      }
      QString mode;
      static_assert(numberOfTradingModes == 3, "List changed.");
      switch (i) {
        case TRADING_MODE_LIVE:
          mode = tr("Live");
          if (!selectedByDefault) {
            selectedByDefault = m_ui.mode->count();
          }
          break;
        case TRADING_MODE_PAPER:
          mode = tr("Paper");
          selectedByDefault = m_ui.mode->count();
          break;
        case TRADING_MODE_BACKTESTING:
          mode = tr("Backtesting");
          break;
        default:
          throw LogicError("Unknown trading mode");
      }
      m_ui.mode->addItem(mode, i);
    }
    if (selectedByDefault) {
      m_ui.mode->setCurrentIndex(*selectedByDefault);
    }
    LoadTradingSystemList(m_ui.mode->currentIndex());
    connect(m_ui.mode, static_cast<void (QComboBox::*)(int)>(
                           &QComboBox::currentIndexChanged),
            this, &OrderWindow::LoadTradingSystemList);
  }

  UpdatePrices(m_security);

  connect(&m_engine, &Engine::StateChanged, this, &OrderWindow::OnStateChanged,
          Qt::QueuedConnection);

  connect(&m_engine.GetDropCopy(), &DropCopy::PriceUpdate, this,
          &OrderWindow::UpdatePrices, Qt::QueuedConnection);

  connect(m_ui.buy, &QPushButton::clicked, this, &OrderWindow::SendBuyOrder);
  connect(m_ui.sell, &QPushButton::clicked, this, &OrderWindow::SendSellOrder);
}

void OrderWindow::OnStateChanged(bool isStarted) { setEnabled(isStarted); }

void OrderWindow::UpdatePrices(const Security &security) {
  if (&security != &m_security) {
    return;
  }

  {
    const auto &lastTime = security.GetLastMarketDataTime();
    if (lastTime != pt::not_a_date_time) {
      const auto &time = lastTime.time_of_day();
      QString text;
      text.sprintf("%02d:%02d:%02d", time.hours(), time.minutes(),
                   time.seconds());
      m_ui.lastTime->setText(text);
    } else {
      m_ui.lastTime->setText("--:--:--");
    }
  }

  {
    const auto &precision = m_security.GetPricePrecision();
    const auto &bid = m_security.GetBidPriceValue();
    const auto &ask = m_security.GetAskPriceValue();
    m_ui.bidPrice->setText(
        QString::number(!isnan(bid) ? bid : 0, 'f', precision));
    m_ui.askPrice->setText(
        QString::number(!isnan(ask) ? ask : 0, 'f', precision));
    m_ui.spread->setText(QString::number(
        !isnan(bid) && !isnan(ask) ? ask - bid : 0, 'f', precision));
  }
}

void OrderWindow::LoadTradingSystemList(int index) {
  m_ui.target->clear();
  const auto &mode =
      static_cast<TradingMode>(m_ui.mode->itemData(index).toInt());
  for (size_t i = 0; i < m_engine.GetContext().GetNumberOfTradingSystems();
       ++i) {
    auto &tradingSystem = m_engine.GetContext().GetTradingSystem(i, mode);
    m_ui.target->addItem(
        QString::fromStdString(tradingSystem.GetInstanceName()), i);
  }
}

TradingSystem &OrderWindow::GetSelectedTradingSystem() {
  return m_engine.GetContext().GetTradingSystem(
      m_ui.target->itemData(m_ui.target->currentIndex()).toUInt(),
      static_cast<TradingMode>(
          m_ui.mode->itemData(m_ui.mode->currentIndex()).toInt()));
}

void OrderWindow::closeEvent(QCloseEvent *closeEvent) {
  Base::closeEvent(closeEvent);
  emit Closed();
}

void OrderWindow::SendBuyOrder() {
  static const OrderParams params;
  auto &tradingSystemMode = GetSelectedTradingSystem();
  for (;;) {
    try {
      tradingSystemMode.Buy(
          m_security, m_security.GetSymbol().GetCurrency(), m_ui.qty->value(),
          m_security.GetAskPrice(), params,
          m_engine.GetOrderTradingSystemSlot(),
          m_engine.GetRiskControl(tradingSystemMode.GetMode()), Milestones());
      break;
    } catch (const std::exception &ex) {
      if (QMessageBox::critical(
              this, tr("Failed to send order"), QString("%1.").arg(ex.what()),
              QMessageBox::Abort | QMessageBox::Retry) != QMessageBox::Retry) {
        break;
      }
    }
  }
}
void OrderWindow::SendSellOrder() {
  static const OrderParams params;
  auto &tradingSystemMode = GetSelectedTradingSystem();
  for (;;) {
    try {
      tradingSystemMode.Sell(
          m_security, m_security.GetSymbol().GetCurrency(), m_ui.qty->value(),
          m_security.GetBidPrice(), params,
          m_engine.GetOrderTradingSystemSlot(),
          m_engine.GetRiskControl(tradingSystemMode.GetMode()), Milestones());
      break;
    } catch (const std::exception &ex) {
      if (QMessageBox::critical(
              this, tr("Failed to send order"), QString("%1.").arg(ex.what()),
              QMessageBox::Abort | QMessageBox::Retry) != QMessageBox::Retry) {
        break;
      }
    }
  }
}
