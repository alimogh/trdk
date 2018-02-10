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
#include "Balances.hpp"
#include "DropCopy.hpp"
#include "OrderStatusHandler.hpp"
#include "Position.hpp"
#include "RiskControl.hpp"
#include "Security.hpp"
#include "Strategy.hpp"
#include "Timer.hpp"
#include "Trade.hpp"
#include "TradingLog.hpp"
#include "TransactionContext.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;

namespace pt = boost::posix_time;

namespace {

const boost::function<bool(OrderTransactionContext &)>
    emptyOrderTransactionContextCallback([](OrderTransactionContext &) {
      return true;
    });

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

template <trdk::Lib::Concurrency::Profile profile>
struct ActiveOrderConcurrencyPolicy {
  static_assert(profile == trdk::Lib::Concurrency::PROFILE_RELAX,
                "Wrong concurrency profile");
  typedef boost::shared_mutex Mutex;
  typedef boost::shared_lock<Mutex> ReadLock;
  typedef boost::unique_lock<Mutex> WriteLock;
};
template <>
struct ActiveOrderConcurrencyPolicy<trdk::Lib::Concurrency::PROFILE_HFT> {
  typedef trdk::Lib::Concurrency::SpinMutex Mutex;
  typedef Mutex::ScopedLock WriteLock;
  typedef Mutex::ScopedLock ReadLock;
};
typedef ActiveOrderConcurrencyPolicy<TRDK_CONCURRENCY_PROFILE>::Mutex
    ActiveOrderMutex;
typedef ActiveOrderConcurrencyPolicy<TRDK_CONCURRENCY_PROFILE>::ReadLock
    ActiveOrderReadLock;
typedef ActiveOrderConcurrencyPolicy<TRDK_CONCURRENCY_PROFILE>::WriteLock
    ActiveOrderWriteLock;
}  // namespace

template <typename Order>
Volume GetCommission(const Order &order,
                     const boost::optional<Volume> commission,
                     const TradingSystem &tradingSystem) {
  return commission ? *commission
                    : order.numberOfTrades
                          ? tradingSystem.CalcCommission(
                                order.qty - order.remainingQty,
                                order.sumOfTradePrices / order.numberOfTrades,
                                order.side, order.security)
                          : 0;
}

class TradingSystem::Implementation : private boost::noncopyable {
 public:
  TradingSystem &m_self;

  const TradingMode m_mode;

  size_t m_index;

  Context &m_context;

  const std::string m_instanceName;
  const std::string m_stringId;

  TradingSystem::Log m_log;
  TradingSystem::TradingLog m_tradingLog;

  ActiveOrderMutex m_activeOrdersMutex;
  Orders m_activeOrders;
  std::unique_ptr<Timer::Scope> m_lastOrderTimerScope;

  explicit Implementation(TradingSystem &self,
                          const TradingMode &mode,
                          Context &context,
                          const std::string &instanceName)
      : m_self(self),
        m_mode(mode),
        m_index(std::numeric_limits<size_t>::max()),
        m_context(context),
        m_instanceName(instanceName),
        m_stringId(FormatStringId(m_instanceName, m_mode)),
        m_log(m_stringId, m_context.GetLog()),
        m_tradingLog(instanceName, m_context.GetTradingLog()) {}

  ~Implementation() {
    try {
      if (!m_activeOrders.empty()) {
        m_log.Warn("%1% orders still be active.", m_activeOrders.size());
      }
    } catch (...) {
      AssertFailNoException();
    }
  }

  void SendOrder(const boost::shared_ptr<Order> &order,
                 const OrderParams &params) {
    ReportNewOrder(*order, "sending");

    order->riskControlOperationId = CheckNewOrder(*order);

    try {
      {
        const ActiveOrderWriteLock lock(m_activeOrdersMutex);
        order->transactionContext = m_self.SendOrderTransaction(
            order->security, order->currency, order->qty, order->price, params,
            order->side, order->tif);
        Assert(order->transactionContext);
        ReportNewOrder(*order, ConvertToPch(ORDER_STATUS_SENT));
        RegisterCallback(order);
      }
      m_self.OnTransactionSent(*order->transactionContext);
    } catch (const std::exception &ex) {
      m_tradingLog.Write(
          "{'order': {'sendError': {'reason': '%1%'}}}",
          [&ex](TradingRecord &record) { record % std::string(ex.what()); });
      try {
        throw;
      } catch (const CommunicationError &ex) {
        m_log.Debug(
            "Communication error while sending order transaction: \"%1%\".",
            ex.what());
      } catch (const std::exception &ex) {
        m_log.Error("Error while sending order transaction: \"%1%\".",
                    ex.what());
      }
      ConfirmOrder(*order, ORDER_STATUS_ERROR, boost::none);
      throw;
    } catch (...) {
      m_tradingLog.Write(
          "{'order': {'sendError': {'reason': 'Unknown exception'}}}");
      m_log.Error("Unknown error while sending order transaction.");
      AssertFailNoException();
      ConfirmOrder(*order, ORDER_STATUS_ERROR, boost::none);
      throw;
    }

    m_self.GetBalancesStorage().ReduceAvailableToTradeByOrder(
        order->security, order->qty, order->actualPrice, order->side, m_self);
  }

  void ReportNewOrder(const Order &order, const char *status) {
    m_tradingLog.Write(
        !order.transactionContext
            ? "{'order': {'new': {'status': '%1%'), 'side': '%2%', 'security': "
              "'%3%', 'currency': '%4%', 'type': '%5%', 'price': %6$.8f, "
              "'qty': %7$.8f, 'tif': '%8%'}}"
            : "{'order': {'new': {'status': '%1%'), 'side': '%2%', 'security': "
              "'%3%', 'currency': '%4%', 'type': '%5%', 'price': %6$.8f, "
              "'qty': %7$.8f, 'tif': '%8%', 'id': '%9%'}}",
        [&](TradingRecord &record) {
          record % status                                             // 1
              % order.side                                            // 2
              % order.security                                        // 3
              % order.currency                                        // 4
              % (order.price ? ORDER_TYPE_LIMIT : ORDER_TYPE_MARKET)  // 5
              % order.actualPrice                                     // 6
              % order.remainingQty                                    // 7
              % order.tif;                                            // 8
          if (order.transactionContext) {
            record % order.transactionContext->GetOrderId();  // 9
          }
        });
  }
  void ReportOrderUpdate(const OrderId &orderId,
                         const Order &order,
                         const char *status,
                         const Volume &commission,
                         const boost::optional<Trade> &trade) {
    m_tradingLog.Write(
        trade ? "{'order': {'trade': {'qty': %1$.8f, 'price': %2$.8f, "
                "'remainingQty': %3$.8f, 'commission': %4$.8f, 'id': '%5%'}, "
                "'status': '%6%', 'side': '%7%', 'security': '%8%', "
                "'currency': '%9%', 'qty': %10$.8f, 'price': %11$.8f, 'tif': "
                "'%12%', 'id': '%13%'}}"
              : "{'order': {'update': {'remainingQty': %1$.8f, 'commission': "
                "%2$.8f}, 'status': '%3%', 'side': '%4%', 'security': '%5%', "
                "'currency': '%6%', 'qty': %7$.8f, 'price': %8$.8f, 'tif': "
                "'%9%', 'id': '%10%'}}",
        [&](TradingRecord &record) {
          if (trade) {
            record % trade->qty  // 1
                % trade->price;  // 2
          }
          record % order.remainingQty  // 3/1
              % commission;            // 4/2
          if (trade) {
            record % trade->id;  // 5
          }
          record % status       // 6/3
              % order.side      // 7/4
              % order.security  // 8/5
              % order.currency  // 9/6
              % order.qty;      // 10/7
          if (order.price) {
            record % *order.price;  // 11/8
          } else {
            record % "";  // 11/8
          }
          record % order.tif  // 12/9
              % orderId;      // 13/10
        });
  }

  RiskControlOperationId CheckNewOrder(const Order &order) {
    const auto &check = order.side == ORDER_SIDE_BUY
                            ? &RiskControl::CheckNewBuyOrder
                            : &RiskControl::CheckNewSellOrder;
    return (m_context.GetRiskControl(m_mode).*check)(
        order.riskControlScope, order.security, order.currency,
        order.remainingQty, order.actualPrice, order.delayMeasurement);
  }

  void ConfirmOrder(const Order &order,
                    const OrderStatus &status,
                    const boost::optional<Trade> &trade) {
    const auto &check = order.side == ORDER_SIDE_BUY
                            ? &RiskControl::ConfirmBuyOrder
                            : &RiskControl::ConfirmSellOrder;
    (m_context.GetRiskControl(m_mode).*check)(
        order.riskControlOperationId, order.riskControlScope, status,
        order.security, order.currency, order.actualPrice, order.remainingQty,
        trade.get_ptr(), order.delayMeasurement);
  }

  void RegisterCallback(const boost::shared_ptr<Order> &order) {
    Assert(order->transactionContext);
    auto result =
        m_activeOrders.emplace(order->transactionContext->GetOrderId(), order);
    if (!result.second) {
      m_log.Error("Order ID %1% is not unique.",
                  order->transactionContext->GetOrderId());
      throw Exception("Order ID is not unique");
    }
    Assert(!order->timerScope);
    if (m_lastOrderTimerScope) {
      order->timerScope = std::move(m_lastOrderTimerScope);
    }
  }

  boost::shared_ptr<Order> GetOrder(const OrderId &orderId) {
    ActiveOrderWriteLock lock(m_activeOrdersMutex);
    auto result = m_activeOrders.find(orderId);
    if (result == m_activeOrders.cend()) {
      throw OrderIsUnknown("Failed to get order as order is unknown");
    }
    return result->second;
  }
  boost::shared_ptr<Order> TakeOrder(const OrderId &orderId) {
    ActiveOrderWriteLock lock(m_activeOrdersMutex);
    auto it = m_activeOrders.find(orderId);
    if (it == m_activeOrders.cend()) {
      throw OrderIsUnknown("Failed to take order is unknown");
    }
    auto result = it->second;
    m_activeOrders.erase(it);
    return result;
  }

  void FinalizeOrder(
      const pt::ptime &time,
      const OrderId &id,
      Order &order,
      OrderStatus status,
      const char *statusName,
      const Qty &remaningQty,
      const boost::optional<Volume> &realCommission,
      boost::optional<Trade> &&trade,
      void (OrderStatusHandler::*handler)(const Volume &),
      const boost::function<bool(trdk::OrderTransactionContext &)> &callback) {
    // You may remove assert from here, it here just to know who will return
    // false.
    Verify(callback(*order.transactionContext));
    AssertLe(remaningQty, order.remainingQty);
    const auto &tradeQty = order.remainingQty - remaningQty;
    order.remainingQty = remaningQty;
    if (trade) {
      if (!trade->price) {
        trade->price = order.actualPrice;
      }
      if (!trade->qty) {
        trade->qty = tradeQty;
      } else if (trade->qty != tradeQty) {
        boost::format error(
            "Wrong trade quantity received. Order remaining quantity %1$.8f, "
            " expected trade quantity %2$.8f and actual trade quantity "
            "%3$.8f.");
        error % order.remainingQty  // 1
            % tradeQty              // 2
            % trade->qty;           // 3
        trade->qty = tradeQty;
        statusName = "error";
        status = ORDER_STATUS_ERROR;
        handler = &OrderStatusHandler::OnError;
      }
      ++order.numberOfTrades;
      order.sumOfTradePrices += trade->price;
      const auto &commission = GetCommission(order, realCommission, m_self);
      ReportOrderUpdate(id, order, statusName, commission, *trade);
      ConfirmOrder(order, status, *trade);
      order.handler->OnTrade(*trade);
      ((*order.handler).*handler)(commission);
      m_context.InvokeDropCopy([&](DropCopy &dropCopy) {
        dropCopy.CopyTrade(time, trade->id, id, m_self, trade->price,
                           trade->qty);
        dropCopy.CopyOrderStatus(id, m_self, time, status, order.remainingQty);
      });
    } else {
      const auto &commission = GetCommission(order, realCommission, m_self);
      ReportOrderUpdate(id, order, statusName, commission, boost::none);
      ConfirmOrder(order, status, boost::none);
      ((*order.handler).*handler)(commission);
      m_context.InvokeDropCopy([&](DropCopy &dropCopy) {
        dropCopy.CopyOrderStatus(id, m_self, time, status, order.remainingQty);
      });
    }
  }

  void FinalizeOrder(
      const pt::ptime &time,
      const OrderId &id,
      Order &order,
      const OrderStatus &status,
      const char *statusName,
      const Qty &remaningQty,
      const boost::optional<Volume> &commission,
      void (OrderStatusHandler::*handler)(const Volume &),
      const boost::function<bool(trdk::OrderTransactionContext &)> &callback) {
    AssertLe(remaningQty, order.remainingQty);
    const auto &tradeQty = order.remainingQty - remaningQty;
    FinalizeOrder(time, id, order, status, statusName, remaningQty, commission,
                  Trade{order.actualPrice, tradeQty}, handler, callback);
  }

  void FinalizeOrder(
      const pt::ptime &time,
      const OrderId &id,
      const OrderStatus &status,
      const char *statusName,
      const boost::optional<Qty> &remainingQty,
      const boost::optional<Volume> &commission,
      void (OrderStatusHandler::*handler)(const Volume &),
      const boost::function<bool(trdk::OrderTransactionContext &)> &callback) {
    const auto &order = TakeOrder(id);
    FinalizeOrder(time, id, *order, status, statusName,
                  remainingQty.get_value_or(order->remainingQty), commission,
                  handler, callback);
  }

  void CancelEmultedIocOrder(const OrderId &orderId,
                             const pt::time_duration &goodInTime) {
    try {
      m_self.CancelOrder(orderId);
    } catch (const CommunicationError &) {
      const ActiveOrderReadLock lock(m_activeOrdersMutex);
      const auto &it = m_activeOrders.find(orderId);
      if (it == m_activeOrders.cend() || !it->second->timerScope) {
        Assert(it == m_activeOrders.cend());
        throw;
      }
      m_context.GetTimer().Schedule(goodInTime,
                                    [this, orderId, goodInTime]() {
                                      CancelEmultedIocOrder(orderId,
                                                            goodInTime);
                                    },
                                    *it->second->timerScope);
    } catch (...) {
      m_log.Error(
          "IOC-emulation for order \"%1%\" is stopped by order canceling "
          "error.",
          orderId);
      throw;
    }
  };
};

TradingSystem::TradingSystem(const TradingMode &mode,
                             Context &context,
                             const std::string &instanceName)
    : m_pimpl(boost::make_unique<Implementation>(
          *this, mode, context, instanceName)) {}

TradingSystem::~TradingSystem() = default;

const TradingMode &TradingSystem::GetMode() const { return m_pimpl->m_mode; }

void TradingSystem::AssignIndex(size_t index) {
  AssertEq(std::numeric_limits<size_t>::max(), m_pimpl->m_index);
  AssertNe(std::numeric_limits<size_t>::max(), index);
  m_pimpl->m_index = index;
}

size_t TradingSystem::GetIndex() const {
  AssertNe(std::numeric_limits<size_t>::max(), m_pimpl->m_index);
  return m_pimpl->m_index;
}

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

const pt::time_duration &TradingSystem::GetDefaultPollingInterval() const {
  static const pt::time_duration result = pt::seconds(3);
  return result;
}

const Balances &TradingSystem::GetBalances() const {
  return const_cast<TradingSystem *>(this)->GetBalancesStorage();
}

Balances &TradingSystem::GetBalancesStorage() {
  static class DummyBalances : public Balances {
   public:
    virtual ~DummyBalances() override = default;

   public:
    virtual Volume FindAvailableToTrade(const std::string &) const override {
      return 0;
    }
    virtual void ReduceAvailableToTradeByOrder(const Security &,
                                               const Qty &,
                                               const Price &,
                                               const OrderSide &,
                                               const TradingSystem &) override {
    }
    virtual void ForEach(
        const boost::function<void(
            const std::string &, const trdk::Volume &, const trdk::Volume &)> &)
        const override {}
  } result;
  return result;
}

boost::optional<TradingSystem::OrderCheckError> TradingSystem::CheckOrder(
    const Security &,
    const Currency &,
    const Qty &,
    const boost::optional<Price> &,
    const OrderSide &) const {
  return boost::none;
}

bool TradingSystem::CheckSymbol(const std::string &) const { return true; }

std::vector<OrderId> TradingSystem::GetActiveOrderIdList() const {
  std::vector<OrderId> result;
  const ActiveOrderReadLock lock(m_pimpl->m_activeOrdersMutex);
  result.reserve(m_pimpl->m_activeOrders.size());
  for (const auto &order : m_pimpl->m_activeOrders) {
    result.emplace_back(order.first);
  }
  return result;
}

const TradingSystem::Orders &TradingSystem::GetActiveOrders() const {
  return m_pimpl->m_activeOrders;
}

void TradingSystem::Connect(const IniSectionRef &conf) {
  if (IsConnected()) {
    return;
  }
  CreateConnection(conf);
}

boost::shared_ptr<const OrderTransactionContext> TradingSystem::SendOrder(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler,
    RiskControlScope &riskControlScope,
    const OrderSide &side,
    const TimeInForce &tif,
    const Milestones &delayMeasurement) {
  Assert(handler);
  const auto &time = GetContext().GetCurrentTime();
  const auto &order = boost::make_shared<Order>(
      Order{security, currency, side, std::move(handler), qty, qty, price,
            price ? *price
                  : side == ORDER_SIDE_BUY ? security.GetAskPrice()
                                           : security.GetBidPrice(),
            ORDER_STATUS_SENT, tif, delayMeasurement, riskControlScope});

  m_pimpl->SendOrder(order, params);

  GetContext().InvokeDropCopy([this, &order, &time](DropCopy &dropCopy) {
    dropCopy.CopySubmittedOrder(order->transactionContext->GetOrderId(), time,
                                order->security, order->currency, *this,
                                order->side, order->qty, order->price,
                                order->tif);
  });

  return order->transactionContext;
}
boost::shared_ptr<const OrderTransactionContext> TradingSystem::SendOrder(
    boost::shared_ptr<Position> &&position,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler,
    const OrderSide &side,
    const TimeInForce &tif,
    const Milestones &delayMeasurement) {
  Assert(handler);
  const auto &time = GetContext().GetCurrentTime();
  auto &security = position->GetSecurity();
  const auto &order = boost::make_shared<Order>(
      Order{security, position->GetCurrency(), side, std::move(handler), qty,
            qty, price,
            price ? *price
                  : side == ORDER_SIDE_BUY ? security.GetAskPrice()
                                           : security.GetBidPrice(),
            false, tif, delayMeasurement,
            position->GetStrategy().GetRiskControlScope(),
            // Order record should hold position object to guarantee that
            // operation end event will be raised only after last order will
            // be canceled or filled:
            std::move(position)});

  m_pimpl->SendOrder(order, params);

  GetContext().InvokeDropCopy([this, &order, &time](DropCopy &dropCopy) {
    dropCopy.CopySubmittedOrder(order->transactionContext->GetOrderId(), time,
                                *order->position, order->side, order->qty,
                                order->price, order->tif);
  });

  return order->transactionContext;
}

void TradingSystem::OnTransactionSent(const OrderTransactionContext &) {}

std::unique_ptr<OrderTransactionContext>
TradingSystem::SendOrderTransactionAndEmulateIoc(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderParams &params,
    const OrderSide &side) {
  auto result = SendOrderTransaction(security, currency, qty, price, params,
                                     side, TIME_IN_FORCE_GTC);
  Assert(!m_pimpl->m_lastOrderTimerScope);
  const auto &orderId = result->GetOrderId();
  m_pimpl->m_lastOrderTimerScope = boost::make_unique<Timer::Scope>();
  const auto goodInTime =
      params.goodInTime ? *params.goodInTime : GetDefaultPollingInterval();
  GetContext().GetTimer().Schedule(goodInTime,
                                   [this, orderId, goodInTime]() {
                                     m_pimpl->CancelEmultedIocOrder(orderId,
                                                                    goodInTime);
                                   },
                                   *m_pimpl->m_lastOrderTimerScope);
  return result;
}

bool TradingSystem::CancelOrder(const OrderId &orderId) {
  GetTradingLog().Write(
      "{'order': {'cancel': {'id': '%1%'}}}",
      [&orderId](TradingRecord &record) { record % orderId; });

  boost::shared_ptr<const OrderTransactionContext> transaction;
  {
    ActiveOrderWriteLock lock(m_pimpl->m_activeOrdersMutex);

    const auto &it = m_pimpl->m_activeOrders.find(orderId);
    if (it == m_pimpl->m_activeOrders.cend()) {
      lock.unlock();
      GetTradingLog().Write(
          "{'order': {'cancelSendError': {'id': '%1%', 'reason': 'Order is "
          "not in the local active order list.'}}}",
          [&orderId](TradingRecord &record) { record % orderId; });
      return false;
    }
    transaction = it->second->transactionContext;
    if (it->second->isCancelRequestSent) {
      lock.unlock();
      GetTradingLog().Write(
          "{'order': {'cancelSendError': {'id': '%1%', 'reason': 'Order "
          "cancel request is already sent.'}}}",
          [&transaction](TradingRecord &record) {
            record % transaction->GetOrderId();
          });
      return false;
    }

    try {
      SendCancelOrderTransaction(*transaction);
    } catch (const OrderIsUnknown &ex) {
      lock.unlock();
      GetTradingLog().Write(
          "{'order': {'cancelSendError': {'id': '%1%', 'reason': '%2%'}}}",
          [&transaction, &ex](TradingRecord &record) {
            record % transaction->GetOrderId()  // 1
                % std::string(ex.what());       // 2
          });
      OnTransactionSent(*transaction);
      return false;
    } catch (const std::exception &ex) {
      lock.unlock();
      GetTradingLog().Write(
          "{'order': {'cancelSendError': {'id': '%1%', 'reason': '%2%'}}}",
          [&transaction, &ex](TradingRecord &record) {
            record % transaction->GetOrderId()  // 1
                % std::string(ex.what());       // 2
          });
      try {
        throw;
      } catch (const CommunicationError &ex) {
        GetLog().Debug(
            "Communication error while sending order cancel transaction for "
            "order \"%1%\": \"%2%\".",
            transaction->GetOrderId(),  // 1
            std::string(ex.what()));    // 2
      } catch (const std::exception &ex) {
        GetLog().Error(
            "Error while sending order cancel transaction for order \"%1%\": "
            "\"%2%\".",
            orderId,                  // 1
            std::string(ex.what()));  // 2
      }
      OnTransactionSent(*transaction);
      throw;
    } catch (...) {
      lock.unlock();
      GetTradingLog().Write(
          "{'order': {'cancelSendError': {'id': '%1%', 'reason': 'Unknown "
          "exception'}}}",
          [&transaction](TradingRecord &record) {
            record % transaction->GetOrderId();
          });
      GetLog().Error(
          "Unknown error while sending order cancel transaction for order "
          "\"%1%\".",
          orderId);
      AssertFailNoException();
      throw;
    }

    it->second->isCancelRequestSent = true;
  }

  GetTradingLog().Write("{'order': {'cancelSent': {'id': '%1%'}}}",
                        [&transaction](TradingRecord &record) {
                          record % transaction->GetOrderId();
                        });

  OnTransactionSent(*transaction);

  return true;
}

void TradingSystem::OnSettingsUpdate(const IniSectionRef &) {}

void TradingSystem::OnOrderOpened(const pt::ptime &time,
                                  const OrderId &orderId) {
  OnOrderOpened(time, orderId, emptyOrderTransactionContextCallback);
}
void TradingSystem::OnOrderOpened(
    const pt::ptime &time,
    const OrderId &id,
    const boost::function<bool(trdk::OrderTransactionContext &)> &callback) {
  const auto &order = m_pimpl->GetOrder(id);
  static_assert(numberOfOrderStatuses == 7, "List changed");
  if (order->isOpened || !callback(*order->transactionContext)) {
    return;
  }

  order->isOpened = true;

  m_pimpl->ConfirmOrder(*order, ORDER_STATUS_OPENED, boost::none);
  order->handler->OnOpened();

  GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {
    dropCopy.CopyOrderStatus(id, *this, time, ORDER_STATUS_OPENED,
                             order->remainingQty);
  });
}

void TradingSystem::OnTrade(const pt::ptime &time,
                            const OrderId &id,
                            Trade &&trade) {
  OnTrade(time, id, std::move(trade), emptyOrderTransactionContextCallback);
}
void TradingSystem::OnTrade(
    const pt::ptime &time,
    const OrderId &orderId,
    Trade &&trade,
    const boost::function<bool(OrderTransactionContext &)> &callback) {
  const auto &order = m_pimpl->GetOrder(orderId);
  // You may remove assert from here, it here just to know who will return
  // false.
  Verify(callback(*order->transactionContext));

  if (!trade.price) {
    trade.price = order->actualPrice;
  }
  if (!trade.qty) {
    trade.qty = order->remainingQty;
  } else if (trade.qty > order->remainingQty) {
    boost::format error(
        "Wrong trade quantity received. Order remaining quantity %1$.8f and "
        "trade quantity %2$.8f (more than order remaining quantity)");
    error % order->remainingQty  // 1
        % trade.qty;             // 2
    OnOrderError(time, orderId, Qty(0), boost::none, error.str());
    return;
  }

  order->isOpened = true;
  order->remainingQty -= trade.qty;
  ++order->numberOfTrades;
  order->sumOfTradePrices += trade.price;

  m_pimpl->ReportOrderUpdate(orderId, *order, "trade", 0, trade);
  m_pimpl->ConfirmOrder(*order, ORDER_STATUS_OPENED, trade);
  order->handler->OnTrade(trade);
  GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {
    dropCopy.CopyOrderStatus(orderId, *this, time, ORDER_STATUS_OPENED,
                             order->remainingQty);
    dropCopy.CopyTrade(time, trade.id, orderId, *this, trade.price, trade.qty);
  });
}

void TradingSystem::OnOrderFilled(const pt::ptime &time,
                                  const OrderId &orderId,
                                  const boost::optional<Volume> &commission) {
  OnOrderFilled(time, orderId, commission,
                emptyOrderTransactionContextCallback);
}
void TradingSystem::OnOrderFilled(
    const pt::ptime &time,
    const OrderId &id,
    const boost::optional<Volume> &commission,
    const boost::function<bool(trdk::OrderTransactionContext &)> &callback) {
  m_pimpl->FinalizeOrder(time, id, *m_pimpl->TakeOrder(id),
                         ORDER_STATUS_FILLED_FULLY, "filled", 0, commission,
                         &OrderStatusHandler::OnFilled, callback);
}

void TradingSystem::OnOrderFilled(const pt::ptime &time,
                                  const OrderId &id,
                                  Trade &&trade,
                                  const boost::optional<Volume> &commission) {
  OnOrderFilled(time, id, std::move(trade), commission,
                emptyOrderTransactionContextCallback);
}
void TradingSystem::OnOrderFilled(
    const pt::ptime &time,
    const OrderId &id,
    Trade &&trade,
    const boost::optional<Volume> &commission,
    const boost::function<bool(trdk::OrderTransactionContext &)> &callback) {
  m_pimpl->FinalizeOrder(
      time, id, *m_pimpl->TakeOrder(id), ORDER_STATUS_FILLED_FULLY, "filled", 0,
      commission, std::move(trade), &OrderStatusHandler::OnFilled, callback);
}

void TradingSystem::OnOrderCanceled(const pt::ptime &time,
                                    const OrderId &id,
                                    const boost::optional<Qty> &remainingQty,
                                    const boost::optional<Volume> &commission) {
  Assert(!remainingQty || *remainingQty > 0);
  m_pimpl->FinalizeOrder(
      time, id, ORDER_STATUS_CANCELED, "canceled", remainingQty, commission,
      &OrderStatusHandler::OnCanceled, emptyOrderTransactionContextCallback);
}

void TradingSystem::OnOrderRejected(const boost::posix_time::ptime &time,
                                    const OrderId &id,
                                    const boost::optional<Qty> &remainingQty,
                                    const boost::optional<Volume> &commission) {
  Assert(!remainingQty || *remainingQty > 0);
  m_pimpl->FinalizeOrder(
      time, id, ORDER_STATUS_REJECTED, "rejected", remainingQty, commission,
      &OrderStatusHandler::OnRejected, emptyOrderTransactionContextCallback);
}

void TradingSystem::OnOrderError(const pt::ptime &time,
                                 const OrderId &id,
                                 const boost::optional<Qty> &remainingQty,
                                 const boost::optional<Volume> &commission,
                                 const std::string &error) {
  GetLog().Error("Order %1% is rejected with the reason: \"%2%\".",
                 id,      // 1
                 error);  // 2
  m_pimpl->FinalizeOrder(time, id, ORDER_STATUS_ERROR, "error", remainingQty,
                         commission, &OrderStatusHandler::OnError,
                         emptyOrderTransactionContextCallback);
}
