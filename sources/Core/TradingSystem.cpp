/**************************************************************************
 *   Created: 2012/07/24 10:07:02
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "TradingSystem.hpp"
#include "RiskControl.hpp"
#include "Security.hpp"
#include "TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;

//////////////////////////////////////////////////////////////////////////

TradingSystem::Error::Error(const char *what) noexcept : Base::Error(what) {}

TradingSystem::OrderParamsError::OrderParamsError(const char *what,
                                                  const Qty &,
                                                  const OrderParams &) noexcept
    : Error(what) {}

TradingSystem::SendingError::SendingError(const char *what) noexcept
    : Error(what) {}

TradingSystem::OrderIsUnknown::OrderIsUnknown(const char *what) noexcept
    : Error(what) {}

TradingSystem::UnknownOrderCancelError::UnknownOrderCancelError(
    const char *what) noexcept
    : OrderIsUnknown(what) {}

TradingSystem::ConnectionDoesntExistError::ConnectionDoesntExistError(
    const char *what) noexcept
    : Error(what) {}

TradingSystem::UnknownAccountError::UnknownAccountError(
    const char *what) noexcept
    : Error(what) {}

TradingSystem::PositionError::PositionError(const char *what) noexcept
    : Error(what) {}

//////////////////////////////////////////////////////////////////////////

namespace {

std::string FormatStringId(const std::string &instanceName,
                           const TradingMode &mode) {
  std::string result("TradingSystem");
  if (!instanceName.empty()) {
    result += '.';
    result += instanceName;
  }
  result += '.';
  result += ConvertToString(mode);
  return result;
}
}

class TradingSystem::Implementation : private boost::noncopyable {
 public:
  const TradingMode m_mode;

  const size_t m_index;

  Context &m_context;

  const std::string m_instanceName;
  const std::string m_stringId;

  TradingSystem::Log m_log;
  TradingSystem::TradingLog m_tradingLog;

  boost::atomic_size_t m_lastOperationId;

  explicit Implementation(const TradingMode &mode,
                          size_t index,
                          Context &context,
                          const std::string &instanceName)
      : m_mode(mode),
        m_index(index),
        m_context(context),
        m_instanceName(instanceName),
        m_stringId(FormatStringId(m_instanceName, m_mode)),
        m_log(m_stringId, m_context.GetLog()),
        m_tradingLog(instanceName, m_context.GetTradingLog()),
        m_lastOperationId(0) {}

  void LogNewOrder(const char *orderType,
                   size_t operationId,
                   Security &security,
                   const Currency &currency,
                   const Qty &qty,
                   const Price &price) {
    m_tradingLog.Write("%1%\t%2%\t%3%\t%4%\tprice=%5%\tqty=%6%",
                       [&](TradingRecord &record) {
                         record % orderType  // 1
                             % operationId   // 2
                             % security      // 3
                             % currency      // 4
                             % price         // 6
                             % qty;          // 5
                       });
  }
  void LogOrderUpdate(size_t operationId,
                      const OrderId &orderId,
                      const std::string &tradingSystemOrderId,
                      const OrderStatus &orderStatus,
                      const Qty &remainingQty,
                      const boost::optional<Volume> &commission,
                      const TradeInfo *trade) {
    m_tradingLog.Write(
        "ORDER STATUS\t%1%\t%2%\tid=%3%/%4%\tremaining=%5%"
        "\tcomm=%6%\ttrade-id=%7%\ttrade-price=%8%\ttrade-qty=%9%",
        [&](TradingRecord &record) {
          record % operationId        // 1
              % orderStatus           // 2
              % orderId               // 3
              % tradingSystemOrderId  // 4
              % remainingQty;         // 5
          if (commission) {
            record % *commission;  // 6
          } else {
            record % '-';  // 6
          }
          if (trade) {
            record % trade->id  // 7
                % trade->price  // 8
                % trade->qty;   // 9
          } else {
            record % '-'  // 7
                % '-'     // 8
                % '-';    // 9
          }
        });
  }

  void ValidateNewOrder(const Qty &qty, const OrderParams &params) {
    if (params.displaySize && *params.displaySize > qty) {
      throw OrderParamsError(
          "Order display size can't be greater then order size", qty, params);
    }

    if (params.goodInSeconds && params.goodTillTime) {
      throw OrderParamsError(
          "Good Next Seconds and Good Till Time can't be used at"
          " the same time",
          qty, params);
    }
  }

  void ValidateNewIocOrder(const Qty &qty, const OrderParams &params) {
    if (params.goodInSeconds || params.goodTillTime) {
      throw OrderParamsError(
          "Good Next Seconds and Good Till Time can't be used for"
          " Immediate Or Cancel (IOC) order",
          qty, params);
    }

    ValidateNewOrder(qty, params);
  }

  RiskControlOperationId CheckNewBuyOrder(
      RiskControlScope &riskControlScope,
      Security &security,
      const Currency &currency,
      const Qty &qty,
      const Price &price,
      const OrderParams &params,
      const TimeMeasurement::Milestones &timeMeasurement) {
    ValidateNewOrder(qty, params);
    return m_context.GetRiskControl(m_mode).CheckNewBuyOrder(
        riskControlScope, security, currency, qty, price, timeMeasurement);
  }

  RiskControlOperationId CheckNewBuyIocOrder(
      RiskControlScope &riskControlScope,
      Security &security,
      const Currency &currency,
      const Qty &qty,
      const Price &price,
      const OrderParams &params,
      const TimeMeasurement::Milestones &timeMeasurement) {
    ValidateNewIocOrder(qty, params);
    return m_context.GetRiskControl(m_mode).CheckNewBuyOrder(
        riskControlScope, security, currency, qty, price, timeMeasurement);
  }

  RiskControlOperationId CheckNewSellOrder(
      RiskControlScope &riskControlScope,
      Security &security,
      const Currency &currency,
      const Qty &qty,
      const Price &price,
      const OrderParams &params,
      const TimeMeasurement::Milestones &timeMeasurement) {
    ValidateNewOrder(qty, params);
    return m_context.GetRiskControl(m_mode).CheckNewSellOrder(
        riskControlScope, security, currency, qty, price, timeMeasurement);
  }

  RiskControlOperationId CheckNewSellIocOrder(
      RiskControlScope &riskControlScope,
      Security &security,
      const Currency &currency,
      const Qty &qty,
      const Price &price,
      const OrderParams &params,
      const TimeMeasurement::Milestones &timeMeasurement) {
    ValidateNewIocOrder(qty, params);
    return m_context.GetRiskControl(m_mode).CheckNewSellOrder(
        riskControlScope, security, currency, qty, price, timeMeasurement);
  }

  void ConfirmBuyOrder(const RiskControlOperationId &riskControlOperationId,
                       RiskControlScope &riskControlScope,
                       const OrderStatus &orderStatus,
                       Security &security,
                       const Currency &currency,
                       const Price &orderPrice,
                       const Qty &remainingQty,
                       const TradeInfo *trade,
                       const TimeMeasurement::Milestones &timeMeasurement) {
    m_context.GetRiskControl(m_mode).ConfirmBuyOrder(
        riskControlOperationId, riskControlScope, orderStatus, security,
        currency, orderPrice, remainingQty, trade, timeMeasurement);
  }

  void ConfirmSellOrder(const RiskControlOperationId &operationId,
                        RiskControlScope &riskControlScope,
                        const OrderStatus &orderStatus,
                        Security &security,
                        const Currency &currency,
                        const Price &orderPrice,
                        const Qty &remainingQty,
                        const TradeInfo *trade,
                        const TimeMeasurement::Milestones &timeMeasurement) {
    m_context.GetRiskControl(m_mode).ConfirmSellOrder(
        operationId, riskControlScope, orderStatus, security, currency,
        orderPrice, remainingQty, trade, timeMeasurement);
  }
};

TradingSystem::TradingSystem(const TradingMode &mode,
                             size_t index,
                             Context &context,
                             const std::string &instanceName)
    : m_pimpl(new Implementation(mode, index, context, instanceName)) {}

TradingSystem::~TradingSystem() { delete m_pimpl; }

const TradingMode &TradingSystem::GetMode() const { return m_pimpl->m_mode; }

size_t TradingSystem::GetIndex() const { return m_pimpl->m_index; }

Context &TradingSystem::GetContext() { return m_pimpl->m_context; }

const Context &TradingSystem::GetContext() const {
  return const_cast<TradingSystem *>(this)->GetContext();
}

TradingSystem::Log &TradingSystem::GetLog() const noexcept {
  return m_pimpl->m_log;
}

TradingSystem::TradingLog &TradingSystem::GetTradingLog() const noexcept {
  return m_pimpl->m_tradingLog;
}

const std::string &TradingSystem::GetInstanceName() const {
  return m_pimpl->m_instanceName;
}

const std::string &TradingSystem::GetStringId() const noexcept {
  return m_pimpl->m_stringId;
}

const TradingSystem::Account &TradingSystem::GetAccount() const {
  throw MethodDoesNotImplementedError("Account Cash Balance not implemented");
}

void TradingSystem::Connect(const IniSectionRef &conf) {
  if (IsConnected()) {
    return;
  }
  CreateConnection(conf);
}

OrderId TradingSystem::SellAtMarketPrice(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const OrderParams &params,
    const OrderStatusUpdateSlot &callback,
    RiskControlScope &riskControlScope,
    const TimeMeasurement::Milestones &timeMeasurement) {
  const auto &supposedPrice = security.GetBidPrice();

  const auto operationId = ++m_pimpl->m_lastOperationId;
  m_pimpl->LogNewOrder("SELL MKT GTC", operationId, security, currency, qty,
                       supposedPrice);

  const auto riskControlOperationId =
      m_pimpl->CheckNewSellOrder(riskControlScope, security, currency, qty,
                                 supposedPrice, params, timeMeasurement);

  try {
    return SendSellAtMarketPrice(
        security, currency, qty, params,
        [&, operationId, riskControlOperationId, currency, supposedPrice,
         timeMeasurement, callback](
            const OrderId &orderId, const std::string &tradingSystemOrderId,
            const OrderStatus &orderStatus, const Qty &remainingQty,
            const boost::optional<Volume> &commission, const TradeInfo *trade) {
          m_pimpl->LogOrderUpdate(operationId, orderId, tradingSystemOrderId,
                                  orderStatus, remainingQty, commission, trade);
          m_pimpl->ConfirmSellOrder(
              riskControlOperationId, riskControlScope, orderStatus, security,
              currency, supposedPrice, remainingQty, trade, timeMeasurement);
          callback(orderId, tradingSystemOrderId, orderStatus, remainingQty,
                   commission, trade);
        });
  } catch (...) {
    GetLog().Warn("Error while sending order, rollback trading check state...");
    m_pimpl->ConfirmSellOrder(riskControlOperationId, riskControlScope,
                              ORDER_STATUS_ERROR, security, currency,
                              supposedPrice, qty, nullptr, timeMeasurement);
    throw;
  }
}

OrderId TradingSystem::Sell(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    const OrderStatusUpdateSlot &callback,
    RiskControlScope &riskControlScope,
    const TimeMeasurement::Milestones &timeMeasurement) {
  const auto operationId = ++m_pimpl->m_lastOperationId;
  m_pimpl->LogNewOrder("SELL LMT GTC", operationId, security, currency, qty,
                       price);

  const auto riskControlOperationId =
      m_pimpl->CheckNewSellOrder(riskControlScope, security, currency, qty,
                                 price, params, timeMeasurement);

  try {
    return SendSell(
        security, currency, qty, price, params,
        [&, operationId, riskControlOperationId, currency, price,
         timeMeasurement, callback](
            const OrderId &orderId, const std::string &tradingSystemOrderId,
            const OrderStatus &orderStatus, const Qty &remainingQty,
            const boost::optional<Volume> &commission, const TradeInfo *trade) {
          m_pimpl->LogOrderUpdate(operationId, orderId, tradingSystemOrderId,
                                  orderStatus, remainingQty, commission, trade);
          m_pimpl->ConfirmSellOrder(riskControlOperationId, riskControlScope,
                                    orderStatus, security, currency, price,
                                    remainingQty, trade, timeMeasurement);
          callback(orderId, tradingSystemOrderId, orderStatus, remainingQty,
                   commission, trade);
        });
  } catch (...) {
    GetLog().Warn("Error while sending order, rollback trading check state...");
    m_pimpl->ConfirmSellOrder(riskControlOperationId, riskControlScope,
                              ORDER_STATUS_ERROR, security, currency, price,
                              qty, nullptr, timeMeasurement);
    throw;
  }
}

OrderId TradingSystem::SellImmediatelyOrCancel(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    const OrderStatusUpdateSlot &callback,
    RiskControlScope &riskControlScope,
    const TimeMeasurement::Milestones &timeMeasurement) {
  const auto operationId = ++m_pimpl->m_lastOperationId;
  m_pimpl->LogNewOrder("SELL LMT IOC", operationId, security, currency, qty,
                       price);

  const auto riskControlOperationId =
      m_pimpl->CheckNewSellIocOrder(riskControlScope, security, currency, qty,
                                    price, params, timeMeasurement);

  try {
    return SendSellImmediatelyOrCancel(
        security, currency, qty, price, params,
        [&, operationId, riskControlOperationId, currency, price,
         timeMeasurement, callback](
            const OrderId &orderId, const std::string &tradingSystemOrderId,
            const OrderStatus &orderStatus, const Qty &remainingQty,
            const boost::optional<Volume> &commission, const TradeInfo *trade) {
          m_pimpl->LogOrderUpdate(operationId, orderId, tradingSystemOrderId,
                                  orderStatus, remainingQty, commission, trade);
          m_pimpl->ConfirmSellOrder(riskControlOperationId, riskControlScope,
                                    orderStatus, security, currency, price,
                                    remainingQty, trade, timeMeasurement);
          callback(orderId, tradingSystemOrderId, orderStatus, remainingQty,
                   commission, trade);
        });
  } catch (...) {
    GetLog().Warn("Error while sending order, rollback trading check state...");
    m_pimpl->ConfirmSellOrder(riskControlOperationId, riskControlScope,
                              ORDER_STATUS_ERROR, security, currency, price,
                              qty, nullptr, timeMeasurement);
    throw;
  }
}

OrderId TradingSystem::SellAtMarketPriceImmediatelyOrCancel(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const OrderParams &params,
    const OrderStatusUpdateSlot &callback,
    RiskControlScope &riskControlScope,
    const TimeMeasurement::Milestones &timeMeasurement) {
  const auto &supposedPrice = security.GetBidPrice();

  const auto operationId = ++m_pimpl->m_lastOperationId;
  m_pimpl->LogNewOrder("SELL MKT IOC", operationId, security, currency, qty,
                       supposedPrice);

  const auto riskControlOperationId =
      m_pimpl->CheckNewSellIocOrder(riskControlScope, security, currency, qty,
                                    supposedPrice, params, timeMeasurement);

  try {
    return SendSellAtMarketPriceImmediatelyOrCancel(
        security, currency, qty, params,
        [&, operationId, riskControlOperationId, currency, supposedPrice,
         timeMeasurement, callback](
            const OrderId &orderId, const std::string &tradingSystemOrderId,
            const OrderStatus &orderStatus, const Qty &remainingQty,
            const boost::optional<Volume> &commission, const TradeInfo *trade) {
          m_pimpl->LogOrderUpdate(operationId, orderId, tradingSystemOrderId,
                                  orderStatus, remainingQty, commission, trade);
          m_pimpl->ConfirmSellOrder(
              riskControlOperationId, riskControlScope, orderStatus, security,
              currency, supposedPrice, remainingQty, trade, timeMeasurement);
          callback(orderId, tradingSystemOrderId, orderStatus, remainingQty,
                   commission, trade);
        });
  } catch (...) {
    GetLog().Warn("Error while sending order, rollback trading check state...");
    m_pimpl->ConfirmSellOrder(riskControlOperationId, riskControlScope,
                              ORDER_STATUS_ERROR, security, currency,
                              supposedPrice, qty, nullptr, timeMeasurement);
    throw;
  }
}

OrderId TradingSystem::BuyAtMarketPrice(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const OrderParams &params,
    const OrderStatusUpdateSlot &callback,
    RiskControlScope &riskControlScope,
    const TimeMeasurement::Milestones &timeMeasurement) {
  const auto &supposedPrice = security.GetAskPrice();

  const auto operationId = ++m_pimpl->m_lastOperationId;
  m_pimpl->LogNewOrder("BUY MKT GTC", operationId, security, currency, qty,
                       supposedPrice);

  const auto riskControlOperationId =
      m_pimpl->CheckNewBuyOrder(riskControlScope, security, currency, qty,
                                supposedPrice, params, timeMeasurement);

  try {
    return SendBuyAtMarketPrice(
        security, currency, qty, params,
        [&, operationId, riskControlOperationId, currency, supposedPrice,
         timeMeasurement, callback](
            const OrderId &orderId, const std::string &tradingSystemOrderId,
            const OrderStatus &orderStatus, const Qty &remainingQty,
            const boost::optional<Volume> &commission, const TradeInfo *trade) {
          m_pimpl->LogOrderUpdate(operationId, orderId, tradingSystemOrderId,
                                  orderStatus, remainingQty, commission, trade);
          m_pimpl->ConfirmBuyOrder(
              riskControlOperationId, riskControlScope, orderStatus, security,
              currency, supposedPrice, remainingQty, trade, timeMeasurement);
          callback(orderId, tradingSystemOrderId, orderStatus, remainingQty,
                   commission, trade);
        });
  } catch (...) {
    GetLog().Debug(
        "Error while sending order, rollback trading check state...");
    m_pimpl->ConfirmBuyOrder(riskControlOperationId, riskControlScope,
                             ORDER_STATUS_ERROR, security, currency,
                             supposedPrice, qty, nullptr, timeMeasurement);
    throw;
  }
}

OrderId TradingSystem::Buy(Security &security,
                           const Currency &currency,
                           const Qty &qty,
                           const Price &price,
                           const OrderParams &params,
                           const OrderStatusUpdateSlot &callback,
                           RiskControlScope &riskControlScope,
                           const TimeMeasurement::Milestones &timeMeasurement) {
  const auto operationId = ++m_pimpl->m_lastOperationId;
  m_pimpl->LogNewOrder("BUY LMT GTC", operationId, security, currency, qty,
                       price);

  const auto riskControlOperationId =
      m_pimpl->CheckNewBuyOrder(riskControlScope, security, currency, qty,
                                price, params, timeMeasurement);

  try {
    return SendBuy(
        security, currency, qty, price, params,
        [&, operationId, riskControlOperationId, currency, price,
         timeMeasurement, callback](
            const OrderId &orderId, const std::string &tradingSystemOrderId,
            const OrderStatus &orderStatus, const Qty &remainingQty,
            const boost::optional<Volume> &commission, const TradeInfo *trade) {
          m_pimpl->LogOrderUpdate(operationId, orderId, tradingSystemOrderId,
                                  orderStatus, remainingQty, commission, trade);
          m_pimpl->ConfirmBuyOrder(riskControlOperationId, riskControlScope,
                                   orderStatus, security, currency, price,
                                   remainingQty, trade, timeMeasurement);
          callback(orderId, tradingSystemOrderId, orderStatus, remainingQty,
                   commission, trade);
        });
  } catch (...) {
    GetLog().Debug(
        "Error while sending order, rollback trading check state...");
    m_pimpl->ConfirmBuyOrder(riskControlOperationId, riskControlScope,
                             ORDER_STATUS_ERROR, security, currency, price, qty,
                             nullptr, timeMeasurement);
    throw;
  }
}

OrderId TradingSystem::BuyImmediatelyOrCancel(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    const OrderStatusUpdateSlot &callback,
    RiskControlScope &riskControlScope,
    const TimeMeasurement::Milestones &timeMeasurement) {
  const auto operationId = ++m_pimpl->m_lastOperationId;
  m_pimpl->LogNewOrder("BUY LMT IOC", operationId, security, currency, qty,
                       price);

  const auto riskControlOperationId =
      m_pimpl->CheckNewBuyIocOrder(riskControlScope, security, currency, qty,
                                   price, params, timeMeasurement);

  try {
    return SendBuyImmediatelyOrCancel(
        security, currency, qty, price, params,
        [&, operationId, riskControlOperationId, currency, price,
         timeMeasurement, callback](
            const OrderId &orderId, const std::string &tradingSystemOrderId,
            const OrderStatus &orderStatus, const Qty &remainingQty,
            const boost::optional<Volume> &commission, const TradeInfo *trade) {
          m_pimpl->LogOrderUpdate(operationId, orderId, tradingSystemOrderId,
                                  orderStatus, remainingQty, commission, trade);
          m_pimpl->ConfirmBuyOrder(riskControlOperationId, riskControlScope,
                                   orderStatus, security, currency, price,
                                   remainingQty, trade, timeMeasurement);
          callback(orderId, tradingSystemOrderId, orderStatus, remainingQty,
                   commission, trade);
        });
  } catch (...) {
    GetLog().Debug(
        "Error while sending order, rollback trading check state...");
    m_pimpl->ConfirmBuyOrder(riskControlOperationId, riskControlScope,
                             ORDER_STATUS_ERROR, security, currency, price, qty,
                             nullptr, timeMeasurement);
    throw;
  }
}

OrderId TradingSystem::BuyAtMarketPriceImmediatelyOrCancel(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const OrderParams &params,
    const OrderStatusUpdateSlot &callback,
    RiskControlScope &riskControlScope,
    const TimeMeasurement::Milestones &timeMeasurement) {
  const auto &supposedPrice = security.GetAskPrice();

  const auto operationId = ++m_pimpl->m_lastOperationId;
  m_pimpl->LogNewOrder("BUY MKT IOC", operationId, security, currency, qty,
                       supposedPrice);

  const auto riskControlOperationId =
      m_pimpl->CheckNewBuyIocOrder(riskControlScope, security, currency, qty,
                                   supposedPrice, params, timeMeasurement);

  try {
    return SendBuyAtMarketPriceImmediatelyOrCancel(
        security, currency, qty, params,
        [&, operationId, riskControlOperationId, currency, supposedPrice,
         timeMeasurement, callback](
            const OrderId &orderId, const std::string &tradingSystemOrderId,
            const OrderStatus &orderStatus, const Qty &remainingQty,
            const boost::optional<Volume> &commission, const TradeInfo *trade) {
          m_pimpl->LogOrderUpdate(operationId, orderId, tradingSystemOrderId,
                                  orderStatus, remainingQty, commission, trade);
          m_pimpl->ConfirmBuyOrder(
              riskControlOperationId, riskControlScope, orderStatus, security,
              currency, supposedPrice, remainingQty, trade, timeMeasurement);
          callback(orderId, tradingSystemOrderId, orderStatus, remainingQty,
                   commission, trade);
        });
  } catch (...) {
    GetLog().Debug(
        "Error while sending order, rollback trading check state...");
    m_pimpl->ConfirmBuyOrder(riskControlOperationId, riskControlScope,
                             ORDER_STATUS_ERROR, security, currency,
                             supposedPrice, qty, nullptr, timeMeasurement);
    throw;
  }
}

void TradingSystem::CancelOrder(const OrderId &order) {
  GetTradingLog().Write("CANCEL ORDER\t%1%",
                        [&](TradingRecord &record) { record % order; });
  SendCancelOrder(order);
}

void TradingSystem::Test() {
  throw MethodDoesNotImplementedError(
      "Trading system does not support testing");
}

void TradingSystem::OnSettingsUpdate(const IniSectionRef &) {}

//////////////////////////////////////////////////////////////////////////

std::ostream &trdk::operator<<(std::ostream &oss,
                               const TradingSystem &tradingSystem) {
  oss << tradingSystem.GetStringId();
  return oss;
}

////////////////////////////////////////////////////////////////////////////////
