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
#include "Lib/Engine.hpp"
#include "Lib/OrderStatusNotifier.hpp"
#include "ui_OrderWindow.h"

using namespace trdk;
using namespace Lib;
using namespace FrontEnd;

namespace {

TradingSystem *GetTradingSystem(const Security &security, Context &context) {
  const auto &instanceName = security.GetSource().GetInstanceName();
  for (size_t i = 0; i < context.GetNumberOfTradingSystems(); ++i) {
    auto &tradingSystem = context.GetTradingSystem(i, TRADING_MODE_LIVE);
    if (tradingSystem.GetInstanceName() == instanceName) {
      return &tradingSystem;
    }
  }
  return nullptr;
}

}  // namespace

class OrderWindow::Implementation {
 public:
  OrderWindow &m_self;
  Ui::OrderWindow m_ui{};
  Engine &m_engine;
  Security &m_security;
  TradingSystem *const m_tradingSystem;

  explicit Implementation(OrderWindow &self,
                          Security &security,
                          FrontEnd::Engine &engine)
      : m_self(self),
        m_engine(engine),
        m_security(security),
        m_tradingSystem(GetTradingSystem(m_security, m_engine.GetContext())) {}
  Implementation(Implementation &&) = default;
  Implementation(const Implementation &) = delete;
  Implementation &operator=(Implementation &&) = delete;
  Implementation &operator=(const Implementation &) = delete;
  ~Implementation() = default;

  void UpdatePrices() {
    m_ui.lastTime->setText(
        ConvertTimeToText(m_security.GetLastMarketDataTime().time_of_day()));
    {
      const auto &bid = m_security.GetBidPriceValue();
      const auto &ask = m_security.GetAskPriceValue();
      m_ui.bidPrice->setText(ConvertPriceToText(bid));
      m_ui.askPrice->setText(ConvertPriceToText(ask));
      const Price spread = bid.IsNotNan() && ask.IsNotNan()
                               ? ask - bid
                               : std::numeric_limits<double>::quiet_NaN();
      m_ui.spread->setText(ConvertPriceToText(spread));
    }
  }

  void SendBuyOrder() {
    if (!m_tradingSystem) {
      QMessageBox::critical(&m_self, tr("Could not send order to buy"),
                            "There is no selected trading system as an order "
                            "target.",
                            QMessageBox::Abort);
      return;
    }
    for (;;) {
      static const OrderParams params{};
      try {
        m_tradingSystem->SendOrder(
            m_security, m_security.GetSymbol().GetCurrency(), m_ui.qty->value(),
            Price{m_ui.price->value()}, params,
            boost::make_unique<OrderStatusNotifier>(),
            m_engine.GetRiskControl(TRADING_MODE_LIVE), ORDER_SIDE_BUY,
            TIME_IN_FORCE_GTC, {});
        break;
      } catch (const std::exception &ex) {
        if (QMessageBox::critical(&m_self, tr("Failed to send order"),
                                  QString("%1.").arg(ex.what()),
                                  QMessageBox::Abort | QMessageBox::Retry) !=
            QMessageBox::Retry) {
          break;
        }
      }
    }
  }

  void SendSellOrder() {
    static const OrderParams params{};
    if (!m_tradingSystem) {
      QMessageBox::critical(&m_self, tr("Could not send order to sell"),
                            "There is no selected trading system as an order "
                            "target.",
                            QMessageBox::Abort);
      return;
    }
    for (;;) {
      try {
        m_tradingSystem->SendOrder(
            m_security, m_security.GetSymbol().GetCurrency(), m_ui.qty->value(),
            Price{m_ui.price->value()}, params,
            boost::make_unique<OrderStatusNotifier>(),
            m_engine.GetRiskControl(TRADING_MODE_LIVE), ORDER_SIDE_SELL,
            TIME_IN_FORCE_GTC, {});
        break;
      } catch (const std::exception &ex) {
        if (QMessageBox::critical(&m_self, tr("Failed to send order"),
                                  QString("%1.").arg(ex.what()),
                                  QMessageBox::Abort | QMessageBox::Retry) !=
            QMessageBox::Retry) {
          break;
        }
      }
    }
  }
};

OrderWindow::OrderWindow(Security &security,
                         FrontEnd::Engine &engine,
                         QWidget *parent)
    : Base(parent),
      m_pimpl(boost::make_unique<Implementation>(*this, security, engine)) {
  m_pimpl->m_ui.setupUi(this);

  m_pimpl->m_ui.qty->setDecimals(m_pimpl->m_security.GetPricePrecision());
  const auto &symbol =
      QString::fromStdString(m_pimpl->m_security.GetSymbol().GetSymbol());
  setWindowTitle(symbol + " - " + QCoreApplication::applicationName());
  m_pimpl->m_ui.symbol->setText(symbol);
  m_pimpl->m_ui.source->setText(
      QString::fromStdString(m_pimpl->m_security.GetSource().GetTitle()));
  m_pimpl->m_ui.fullSymbol->setText(
      QString::fromStdString(m_pimpl->m_security.GetSymbol().GetAsString()));
  m_pimpl->UpdatePrices();

  Verify(connect(&m_pimpl->m_engine, &FrontEnd::Engine::StateChange, this,
                 [this](const bool isStarted) {
                   if (isStarted) {
                     return;
                   }
                   setEnabled(false);
                 },
                 Qt::QueuedConnection));

  Verify(connect(&m_pimpl->m_engine, &FrontEnd::Engine::PriceUpdate, this,
                 [this](const Security *security) {
                   if (&m_pimpl->m_security != security) {
                     return;
                   }
                   m_pimpl->UpdatePrices();
                 }));

  Verify(connect(m_pimpl->m_ui.buy, &QPushButton::clicked,
                 [this]() { m_pimpl->SendBuyOrder(); }));
  Verify(connect(m_pimpl->m_ui.sell, &QPushButton::clicked,
                 [this]() { m_pimpl->SendSellOrder(); }));

  adjustSize();
}
OrderWindow::~OrderWindow() = default;

void OrderWindow::closeEvent(QCloseEvent *event) {
  emit Closed();
  Base::closeEvent(event);
}
