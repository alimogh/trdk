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
#include "OrderWindow.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Security.hpp"
#include "Lib/DropCopy.hpp"
#include "Lib/Engine.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::FrontEnd;
using namespace trdk::FrontEnd::Lib;
using namespace trdk::FrontEnd::Shell;

namespace sh = trdk::FrontEnd::Shell;
namespace pt = boost::posix_time;

OrderWindow::OrderWindow(FrontEnd::Lib::Engine &engine, QWidget *parent)
    : Base(parent), m_engine(engine), m_security(nullptr) {
  m_ui.setupUi(this);
  setEnabled(false);

  if (m_engine.GetContext().GetNumberOfTradingSystems() > 0) {
    boost::optional<int> selectedByDefault;
    for (int i = 0; i < numberOfTradingModes; ++i) {
      try {
        m_engine.GetContext().GetTradingSystem(0, static_cast<TradingMode>(i));
      } catch (const Context::TradingModeIsNotLoaded &) {
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
          throw trdk::Lib::LogicError("Unknown trading mode");
      }
      m_ui.mode->addItem(mode, i);
    }
    if (selectedByDefault) {
      m_ui.mode->setCurrentIndex(*selectedByDefault);
    }
    LoadTradingSystemList(m_ui.mode->currentIndex());
    Verify(connect(m_ui.mode, static_cast<void (QComboBox::*)(int)>(
                                  &QComboBox::currentIndexChanged),
                   this, &OrderWindow::LoadTradingSystemList));
  }

  Verify(connect(m_ui.mode, static_cast<void (QComboBox::*)(int)>(
                                &QComboBox::currentIndexChanged),
                 this, &OrderWindow::LoadTradingSystemList));

  Verify(connect(&m_engine, &Lib::Engine::StateChanged, this,
                 &OrderWindow::OnStateChanged, Qt::QueuedConnection));

  Verify(connect(&m_engine.GetDropCopy(), &Lib::DropCopy::PriceUpdate, this,
                 &OrderWindow::UpdatePrices, Qt::QueuedConnection));

  Verify(connect(m_ui.buy, &QPushButton::clicked, this,
                 &OrderWindow::SendBuyOrder));
  Verify(connect(m_ui.sell, &QPushButton::clicked, this,
                 &OrderWindow::SendSellOrder));

  adjustSize();
}

void OrderWindow::SetSecurity(Security &security) {
  if (m_security == &security) {
    Assert(isEnabled());
    return;
  }
  m_security = &security;
  m_ui.qty->setDecimals(m_security->GetPricePrecision());
  const auto &symbol =
      QString::fromStdString(m_security->GetSymbol().GetSymbol());
  setWindowTitle(symbol + " - " + QCoreApplication::applicationName());
  m_ui.symbol->setText(symbol);
  m_ui.source->setText(
      QString::fromStdString(m_security->GetSource().GetInstanceName()));
  m_ui.fullSymbol->setText(
      QString::fromStdString(m_security->GetSymbol().GetAsString()));
  SelectTradingSystem();
  UpdatePrices(m_security);
  setEnabled(true);
}

void OrderWindow::SelectTradingSystem() {
  Assert(m_security);
  const auto &mode = GetSelectedTradingMode();
  const auto &preferedTradingSystemName =
      m_security->GetSource().GetInstanceName();
  for (int i = 0; i < m_ui.target->count(); ++i) {
    const auto &tradingSystem = m_engine.GetContext().GetTradingSystem(
        m_ui.target->itemData(i).toUInt(), mode);
    if (tradingSystem.GetInstanceName() == preferedTradingSystemName) {
      m_ui.target->setCurrentIndex(i);
      return;
    }
  }
  m_ui.target->setCurrentIndex(-1);
  QMessageBox::warning(
      this, tr("Could not find related trading system"),
      tr("Security \"%1\" from source \"%2\" does not have related trading "
         "system. Please choose a trading system as a target for orders before "
         "sending an order.")
          .arg(QString::fromStdString(m_security->GetSymbol().GetSymbol()),
               QString::fromStdString(preferedTradingSystemName)),
      QMessageBox::Ok);
}

void OrderWindow::OnStateChanged(bool isStarted) {
  if (isStarted) {
    return;
  }
  setEnabled(false);
}

void OrderWindow::UpdatePrices(const Security *security) {
  Assert(security);
  if (security != m_security) {
    return;
  }
  m_ui.lastTime->setText(
      ConvertTimeToText(security->GetLastMarketDataTime().time_of_day()));
  {
    const auto &precision = security->GetPricePrecision();
    const auto &bid = security->GetBidPriceValue();
    const auto &ask = security->GetAskPriceValue();
    m_ui.bidPrice->setText(ConvertPriceToText(bid, precision));
    m_ui.askPrice->setText(ConvertPriceToText(ask, precision));
    const Price spread = bid.IsNotNan() && ask.IsNotNan()
                             ? ask - bid
                             : std::numeric_limits<double>::quiet_NaN();
    m_ui.spread->setText(ConvertPriceToText(spread, precision));
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

TradingSystem *OrderWindow::GetSelectedTradingSystem() {
  const auto &index = m_ui.target->currentIndex();
  if (index < 0) {
    return nullptr;
  }
  return &m_engine.GetContext().GetTradingSystem(
      m_ui.target->itemData(index).toUInt(), GetSelectedTradingMode());
}

TradingMode OrderWindow::GetSelectedTradingMode() const {
  AssertLe(0, m_ui.mode->currentIndex());
  AssertGt(numberOfTradingModes, m_ui.mode->currentIndex());
  return static_cast<TradingMode>(m_ui.mode->currentIndex());
}

void OrderWindow::closeEvent(QCloseEvent *closeEvent) {
  Base::closeEvent(closeEvent);
  emit Closed();
}

void OrderWindow::SendBuyOrder() {
  Assert(m_security);
  static const OrderParams params;
  auto *const tradingSystemMode = GetSelectedTradingSystem();
  if (!tradingSystemMode) {
    QMessageBox::warning(this, tr("Could not send order to buy"),
                         "There is no selected trading system as an order "
                         "target. Please choose a trading mode and a trading "
                         "system and try again.",
                         QMessageBox::Ok);
    return;
  }
  for (;;) {
    try {
      if (IsIocOrder()) {
        tradingSystemMode->BuyImmediatelyOrCancel(
            *m_security, m_security->GetSymbol().GetCurrency(),
            m_ui.qty->value(), m_security->GetAskPrice(), params,
            m_engine.GetOrderTradingSystemSlot(),
            m_engine.GetRiskControl(tradingSystemMode->GetMode()),
            Milestones());
      } else {
        tradingSystemMode->Buy(
            *m_security, m_security->GetSymbol().GetCurrency(),
            m_ui.qty->value(), m_security->GetAskPrice(), params,
            m_engine.GetOrderTradingSystemSlot(),
            m_engine.GetRiskControl(tradingSystemMode->GetMode()),
            Milestones());
      }
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
  Assert(m_security);
  static const OrderParams params;
  auto *const tradingSystemMode = GetSelectedTradingSystem();
  if (!tradingSystemMode) {
    QMessageBox::warning(this, tr("Could not send order to sell"),
                         "There is no selected trading system as an order "
                         "target. Please choose a trading mode and a trading "
                         "system and try again.",
                         QMessageBox::Ok);
    return;
  }
  for (;;) {
    try {
      if (IsIocOrder()) {
        tradingSystemMode->SellImmediatelyOrCancel(
            *m_security, m_security->GetSymbol().GetCurrency(),
            m_ui.qty->value(), m_security->GetBidPrice(), params,
            m_engine.GetOrderTradingSystemSlot(),
            m_engine.GetRiskControl(tradingSystemMode->GetMode()),
            Milestones());
      } else {
        tradingSystemMode->Sell(
            *m_security, m_security->GetSymbol().GetCurrency(),
            m_ui.qty->value(), m_security->GetBidPrice(), params,
            m_engine.GetOrderTradingSystemSlot(),
            m_engine.GetRiskControl(tradingSystemMode->GetMode()),
            Milestones());
      }
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

bool OrderWindow::IsIocOrder() const {
  return m_ui.orderType->currentText() == "IOC";
}
