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

struct ActiveOrder {
  TradingSystem::OrderStatusUpdateSlot statusUpdateSignal;
  Qty remainingQty;
};
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

  boost::unordered_map<OrderId, ActiveOrder> m_activeOrders;

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

  ~Implementation() {
    try {
      if (!m_activeOrders.empty()) {
        m_log.Warn("%1% orders still be active.", m_activeOrders.size());
      }
    } catch (...) {
      AssertFailNoException();
    }
  }

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

  void RegisterCallback(const OrderId &orderId,
                        const OrderStatusUpdateSlot &&callback,
                        const Qty &qty) {
    if (!m_activeOrders.emplace(orderId, ActiveOrder{std::move(callback), qty})
             .second) {
      m_log.Error("Order ID %1% is not unique.", orderId);
      throw Error("Order ID %1% is not unique");
    }
  }

  void OnOrderStatusUpdate(const OrderId &orderId,
                           const std::string &tradingSystemOrderId,
                           const OrderStatus &status,
                           const boost::optional<Qty> &remainingQty,
                           const boost::optional<Volume> &commission,
                           const TradeInfo *tradeInfo) {
    const auto &it = m_activeOrders.find(orderId);
    if (it == m_activeOrders.cend()) {
      m_log.Warn(
          "Failed to handle status update for order %1% as order is unknown.",
          orderId);
      throw OrderIsUnknown(
          "Failed to handle status update for order as order is unknown");
    }

    if (remainingQty) {
      it->second.remainingQty = *remainingQty;
    }

    // Maybe in the future new thread will be required here to avoid deadlocks
    // from callback (when some one will need to call the same trading system
    // from this callback).
    it->second.statusUpdateSignal(orderId, tradingSystemOrderId, status,
                                  it->second.remainingQty, commission,
                                  tradeInfo);

    static_assert(numberOfOrderStatuses == 8, "List changed.");
    switch (status) {
      case ORDER_STATUS_FILLED:
        AssertEq(0, it->second.remainingQty);
      case ORDER_STATUS_CANCELLED:
      case ORDER_STATUS_REJECTED:
      case ORDER_STATUS_ERROR:
        m_activeOrders.erase(it);
        break;
    }
  }
};

TradingSystem::TradingSystem(const TradingMode &mode,
                             size_t index,
                             Context &context,
                             const std::string &instanceName)
    : m_pimpl(boost::make_unique<Implementation>(
          mode, index, context, instanceName)) {}

TradingSystem::~TradingSystem() = default;

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
  throw MethodIsNotImplementedException("Account Cash Balance not implemented");
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
    try {
      throw;
    } catch (const std::exception &ex) {
      GetLog().Warn("Error while sending order: \"%1%\".", ex.what());
    } catch (...) {
      GetLog().Error("Unknown error while sending order.");
    }
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
    try {
      throw;
    } catch (const std::exception &ex) {
      GetLog().Warn("Error while sending order: \"%1%\".", ex.what());
    } catch (...) {
      GetLog().Error("Unknown error while sending order.");
    }
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
    try {
      throw;
    } catch (const std::exception &ex) {
      GetLog().Warn("Error while sending order: \"%1%\".", ex.what());
    } catch (...) {
      GetLog().Error("Unknown error while sending order.");
    }
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
    try {
      throw;
    } catch (const std::exception &ex) {
      GetLog().Warn("Error while sending order: \"%1%\".", ex.what());
    } catch (...) {
      GetLog().Error("Unknown error while sending order.");
    }
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
    try {
      throw;
    } catch (const std::exception &ex) {
      GetLog().Warn("Error while sending order: \"%1%\".", ex.what());
    } catch (...) {
      GetLog().Error("Unknown error while sending order.");
    }
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
    try {
      throw;
    } catch (const std::exception &ex) {
      GetLog().Warn("Error while sending order: \"%1%\".", ex.what());
    } catch (...) {
      GetLog().Error("Unknown error while sending order.");
    }
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
    try {
      throw;
    } catch (const std::exception &ex) {
      GetLog().Warn("Error while sending order: \"%1%\".", ex.what());
    } catch (...) {
      GetLog().Error("Unknown error while sending order.");
    }
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
    try {
      throw;
    } catch (const std::exception &ex) {
      GetLog().Warn("Error while sending order: \"%1%\".", ex.what());
    } catch (...) {
      GetLog().Error("Unknown error while sending order.");
    }
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
  throw MethodIsNotImplementedException(
      "Trading system does not support testing");
}

void TradingSystem::OnSettingsUpdate(const IniSectionRef &) {}

OrderId TradingSystem::SendSellAtMarketPrice(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const OrderParams &params,
    const OrderStatusUpdateSlot &&callback) {
  const auto &orderId = SendSellAtMarketPrice(security, currency, qty, params);
  m_pimpl->RegisterCallback(orderId, std::move(callback), qty);
  return orderId;
}

OrderId TradingSystem::SendSell(Security &security,
                                const Currency &currency,
                                const Qty &qty,
                                const Price &price,
                                const OrderParams &params,
                                const OrderStatusUpdateSlot &&callback) {
  const auto &orderId = SendSell(security, currency, qty, price, params);
  m_pimpl->RegisterCallback(orderId, std::move(callback), qty);
  return orderId;
}

OrderId TradingSystem::SendSellImmediatelyOrCancel(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    const OrderStatusUpdateSlot &&callback) {
  const auto &orderId =
      SendSellImmediatelyOrCancel(security, currency, qty, price, params);
  m_pimpl->RegisterCallback(orderId, std::move(callback), qty);
  return orderId;
}

OrderId TradingSystem::SendSellAtMarketPriceImmediatelyOrCancel(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const OrderParams &params,
    const OrderStatusUpdateSlot &&callback) {
  const auto &orderId =
      SendSellAtMarketPriceImmediatelyOrCancel(security, currency, qty, params);
  m_pimpl->RegisterCallback(orderId, std::move(callback), qty);
  return orderId;
}

OrderId TradingSystem::SendBuyAtMarketPrice(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const OrderParams &params,
    const OrderStatusUpdateSlot &&callback) {
  const auto &orderId = SendBuyAtMarketPrice(security, currency, qty, params);
  m_pimpl->RegisterCallback(orderId, std::move(callback), qty);
  return orderId;
}

OrderId TradingSystem::SendBuy(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    const TradingSystem::OrderStatusUpdateSlot &&callback) {
  const auto &orderId = SendBuy(security, currency, qty, price, params);
  m_pimpl->RegisterCallback(orderId, std::move(callback), qty);
  return orderId;
}

OrderId TradingSystem::SendBuyImmediatelyOrCancel(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    const OrderStatusUpdateSlot &&callback) {
  const auto &orderId =
      SendBuyImmediatelyOrCancel(security, currency, qty, price, params);
  m_pimpl->RegisterCallback(orderId, std::move(callback), qty);
  return orderId;
}

OrderId TradingSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const OrderParams &params,
    const OrderStatusUpdateSlot &&callback) {
  const auto &orderId =
      SendBuyAtMarketPriceImmediatelyOrCancel(security, currency, qty, params);
  m_pimpl->RegisterCallback(orderId, std::move(callback), qty);
  return orderId;
}

void TradingSystem::OnOrderStatusUpdate(const OrderId &orderId,
                                        const std::string &tradingSystemOrderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty,
                                        const Volume &commission,
                                        const TradeInfo &trade) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId, status,
                               remainingQty, commission, &trade);
}
void TradingSystem::OnOrderStatusUpdate(const OrderId &orderId,
                                        const std::string &tradingSystemOrderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId, status,
                               remainingQty, boost::none, nullptr);
}
void TradingSystem::OnOrderStatusUpdate(const OrderId &orderId,
                                        const std::string &tradingSystemOrderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty,
                                        const Volume &commission) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId, status,
                               remainingQty, commission, nullptr);
}
void TradingSystem::OnOrderStatusUpdate(const OrderId &orderId,
                                        const std::string &tradingSystemOrderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty,
                                        const TradeInfo &trade) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId, status,
                               remainingQty, boost::none, &trade);
}

void TradingSystem::OnOrderError(const OrderId &orderId,
                                 const std::string &tradingSystemOrderId) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId,
                               ORDER_STATUS_ERROR, boost::none, boost::none,
                               nullptr);
}

void TradingSystem::OnOrderReject(const OrderId &orderId,
                                  const std::string &tradingSystemOrderId) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId,
                               ORDER_STATUS_REJECTED, boost::none, boost::none,
                               nullptr);
}

////////////////////////////////////////////////////////////////////////////////

LegacyTradingSystem::LegacyTradingSystem(const TradingMode &tradingMode,
                                         size_t index,
                                         Context &context,
                                         const std::string &instanceName)
    : TradingSystem(tradingMode, index, context, instanceName) {}

LegacyTradingSystem::~LegacyTradingSystem() = default;

OrderId LegacyTradingSystem::SendSellAtMarketPrice(Security &,
                                                   const Currency &,
                                                   const Qty &,
                                                   const OrderParams &) {
  AssertFail(
      "Internal error of legacy trading system implementation: this method "
      "never should be called, as legacy trading system use own methods with "
      "callback argument.");
  throw Error("Internal error of legacy trading system implementation");
}

OrderId LegacyTradingSystem::SendSell(Security &,
                                      const Currency &,
                                      const Qty &,
                                      const Price &,
                                      const OrderParams &) {
  AssertFail(
      "Internal error of legacy trading system implementation: this method "
      "never should be called, as legacy trading system use own methods with "
      "callback argument.");
  throw Error("Internal error of legacy trading system implementation");
}

OrderId LegacyTradingSystem::SendSellImmediatelyOrCancel(Security &,
                                                         const Currency &,
                                                         const Qty &,
                                                         const Price &,
                                                         const OrderParams &) {
  AssertFail(
      "Internal error of legacy trading system implementation: this method "
      "never should be called, as legacy trading system use own methods with "
      "callback argument.");
  throw Error("Internal error of legacy trading system implementation");
}

OrderId LegacyTradingSystem::SendSellAtMarketPriceImmediatelyOrCancel(
    Security &, const Currency &, const Qty &, const OrderParams &) {
  AssertFail(
      "Internal error of legacy trading system implementation: this method "
      "never should be called, as legacy trading system use own methods with "
      "callback argument.");
  throw Error("Internal error of legacy trading system implementation");
}

OrderId LegacyTradingSystem::SendBuyAtMarketPrice(Security &,
                                                  const Currency &,
                                                  const Qty &,
                                                  const OrderParams &) {
  AssertFail(
      "Internal error of legacy trading system implementation: this method "
      "never should be called, as legacy trading system use own methods with "
      "callback argument.");
  throw Error("Internal error of legacy trading system implementation");
}

OrderId LegacyTradingSystem::SendBuy(Security &,
                                     const Currency &,
                                     const Qty &,
                                     const Price &,
                                     const OrderParams &) {
  AssertFail(
      "Internal error of legacy trading system implementation: this method "
      "never should be called, as legacy trading system use own methods with "
      "callback argument.");
  throw Error("Internal error of legacy trading system implementation");
}

OrderId LegacyTradingSystem::SendBuyImmediatelyOrCancel(Security &,
                                                        const Currency &,
                                                        const Qty &,
                                                        const Price &,
                                                        const OrderParams &) {
  AssertFail(
      "Internal error of legacy trading system implementation: this method "
      "never should be called, as legacy trading system use own methods with "
      "callback argument.");
  throw Error("Internal error of legacy trading system implementation");
}

OrderId LegacyTradingSystem::SendBuyAtMarketPriceImmediatelyOrCancel(
    Security &, const Currency &, const Qty &, const OrderParams &) {
  AssertFail(
      "Internal error of legacy trading system implementation: this method "
      "never should be called, as legacy trading system use own methods with "
      "callback argument.");
  throw Error("Internal error of legacy trading system implementation");
}

//////////////////////////////////////////////////////////////////////////

std::ostream &trdk::operator<<(std::ostream &oss,
                               const TradingSystem &tradingSystem) {
  oss << tradingSystem.GetStringId();
  return oss;
}

////////////////////////////////////////////////////////////////////////////////
