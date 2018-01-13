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
#include "RiskControl.hpp"
#include "Security.hpp"
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
    emptyOrderTransactionContextCallback;

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
}

bool TradingSystem::Order::IsChanged(
    const OrderStatus &rhsStatus,
    const boost::optional<Qty> &rhsRemainingQty,
    const boost::optional<Trade> &trade) const {
  return trade || status != rhsStatus ||
         (rhsRemainingQty && remainingQty != *rhsRemainingQty);
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

  void ReportNewOrder(const Order &order,
                      const char *status,
                      const boost::optional<Price> &orderPrice) {
    m_tradingLog.Write(
        !order.transactionContext
            ? "{'order': {'new': {'status': '%1%'), 'side': '%2%', 'security': "
              "'%3%', 'currency': '%4%', 'type': '%5%', 'price': %6$.8f, "
              "'qty': %7$.8f, 'tif': '%8%'}}"
            : "{'order': {'new': {'status': '%1%'), 'side': '%2%', 'security': "
              "'%3%', 'currency': '%4%', 'type': '%5%', 'price': %6$.8f, "
              "'qty': %7$.8f, 'tif': '%8%', 'id': '%9%'}}",
        [&](TradingRecord &record) {
          record % status                                            // 1
              % order.side                                           // 2
              % order.security                                       // 3
              % order.currency                                       // 4
              % (orderPrice ? ORDER_TYPE_LIMIT : ORDER_TYPE_MARKET)  // 5
              % order.actualPrice                                    // 6
              % order.remainingQty                                   // 7
              % order.tif;                                           // 8
          if (order.transactionContext) {
            record % order.transactionContext->GetOrderId();  // 9
          }
        });
  }
  void ReportOrderUpdate(const OrderId &orderId,
                         const Order &order,
                         const OrderStatus &status,
                         const Qty &remainingQty,
                         const boost::optional<Volume> &commission,
                         const boost::optional<Trade> &trade) {
    m_tradingLog.Write(
        trade ? "{'order': {'trade': {'qty': %1$.8f, 'price': %2$.8f, "
                "'status': '%3%', 'remainingQty': %4$.8f, 'commission': "
                "%5$.8f, 'id': '%6%'}, 'status': '%7%', 'side': '%8%', "
                "'security': '%9%', 'currency': '%10%', 'qty': %11$.8f, "
                "'price': %12$.8f, 'tif': '%13%', 'id': '%14%'}}"
              : "{'order': {'update': {'status': '%1%', 'remainingQty': "
                "%2$.8f, 'commission': %3$.8f}, 'status': '%4%', 'side': "
                "'%5%', 'security': '%6%', 'currency': '%7%', 'qty': %8$.8f, "
                "'price': %9$.8f, 'tif': '%10%', 'id': '%11%'}}",
        [&](TradingRecord &record) {
          if (trade) {
            record % trade->qty  // 1
                % trade->price;  // 2
          }
          record % status      // 3/1
              % remainingQty;  // 4/2
          if (commission) {
            record % *commission;  // 5/3
          } else {
            record % "null";  // 5/3
          }
          if (trade) {
            record % trade->id;  // 6
          }
          record % order.status  // 7/4
              % order.side       // 8/5
              % order.security   // 9/6
              % order.currency   // 10/7
              % order.qty;       // 11/8
          if (order.price) {
            record % *order.price;  // 12/9
          } else {
            record % "";  // 12/9
          }
          record % order.tif  // 13/10
              % orderId;      // 14/11
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
                    const Qty remainingQty,
                    const boost::optional<Trade> &trade) {
    const auto &check = order.side == ORDER_SIDE_BUY
                            ? &RiskControl::ConfirmBuyOrder
                            : &RiskControl::ConfirmSellOrder;
    (m_context.GetRiskControl(m_mode).*check)(
        order.riskControlOperationId, order.riskControlScope, status,
        order.security, order.currency, order.actualPrice, remainingQty,
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

  void OnOrderStatusUpdate(
      const pt::ptime &time,
      const OrderId &orderId,
      const OrderStatus &status,
      boost::optional<Qty> &&remainingQty,
      const boost::optional<Volume> &commission,
      boost::optional<Trade> &&trade,
      const boost::function<bool(OrderTransactionContext &)> &pred) {
    ActiveOrderWriteLock lock(m_activeOrdersMutex);
    const auto &it = m_activeOrders.find(orderId);
    if (it == m_activeOrders.cend()) {
      throw OrderIsUnknown(
          "Failed to handle status update for order as order is unknown");
    }
    OnOrderStatusUpdate(time, orderId, status, std::move(remainingQty),
                        commission, std::move(trade), it, std::move(lock),
                        pred);
  }

  void OnOrderStatusUpdate(
      const pt::ptime &time,
      const OrderId &orderId,
      const OrderStatus &remoteStatus,
      boost::optional<Qty> &&remainingQty,
      const boost::optional<Volume> &commission,
      boost::optional<Trade> &&trade,
      const Orders::iterator &it,
      ActiveOrderWriteLock &&lock,
      const boost::function<bool(OrderTransactionContext &)> &pred) {
    auto &order = *it->second;

    AssertNe(ORDER_STATUS_FILLED_FULLY, order.status);
    AssertNe(ORDER_STATUS_CANCELED, order.status);
    AssertNe(ORDER_STATUS_REJECTED, order.status);
    AssertNe(ORDER_STATUS_ERROR, order.status);

    if (!order.IsChanged(remoteStatus, remainingQty, trade) ||
        (pred && !pred(*order.transactionContext))) {
      return;
    }
    auto status = remoteStatus;

    Qty actualRemainingQty;
    if (remainingQty) {
      actualRemainingQty = *remainingQty;
      if (order.remainingQty > actualRemainingQty) {
        static_assert(numberOfOrderStatuses == 7, "List changed.");
        switch (status) {
          case ORDER_STATUS_SENT:
          case ORDER_STATUS_OPENED:
            if (actualRemainingQty != 0) {
              status = ORDER_STATUS_FILLED_PARTIALLY;
              break;
            }
          default:
            if (actualRemainingQty == 0) {
              m_log.Error(
                  "Wrong order remaining quantity and/or status received. "
                  "Received remaining quantity 0 (zero) and status \"%1%\".",
                  status);
              AssertNe(0, actualRemainingQty);
              status = ORDER_STATUS_ERROR;
            }
            break;
          case ORDER_STATUS_FILLED_FULLY:
            if (actualRemainingQty != 0) {
              m_log.Error(
                  "Wrong order remaining quantity and/or status received. "
                  "Received remaining quantity %1$.8f (not zero) and status "
                  "\"%2%\".",
                  actualRemainingQty,  // 1
                  status);             // 2
              AssertNe(ORDER_STATUS_FILLED_FULLY, status);
              status = ORDER_STATUS_ERROR;
            }
            break;
        }
      } else if (order.remainingQty < actualRemainingQty) {
        m_log.Error(
            "Wrong order remaining quantity received - greater than was "
            "before. Was %1$.8f but now is %2$.8f. Ignoring order "
            "remaining quantity update.",
            order.remainingQty,   // 1
            actualRemainingQty);  // 2
        AssertGe(order.remainingQty, actualRemainingQty);
        actualRemainingQty = order.remainingQty;
        status = ORDER_STATUS_ERROR;
      }
    } else if (trade && trade->qty) {
      if (order.remainingQty < trade->qty) {
        m_log.Error(
            "Wrong trade quantity received - greater than was order remaining "
            "quantity. Received trade with quantity %1$.8f, but order "
            "remaining quantity is %2$.8f.",
            trade->qty,           // 1
            order.remainingQty);  // 2
        AssertGe(order.remainingQty, trade->qty);
        status = ORDER_STATUS_ERROR;
      }
      actualRemainingQty = order.remainingQty - trade->qty;
      if (actualRemainingQty == 0 && status == ORDER_STATUS_FILLED_PARTIALLY) {
        status = ORDER_STATUS_FILLED_FULLY;
      }
    } else {
      static_assert(numberOfOrderStatuses == 7, "List changed.");
      switch (status) {
        case ORDER_STATUS_FILLED_PARTIALLY:
          m_log.Error(
              "Wrong order remaining quantity and/or status received. "
              "Received remaining quantity 0 (zero) and status \"%1%\".",
              status);
          status = ORDER_STATUS_ERROR;
          AssertNe(ORDER_STATUS_FILLED_PARTIALLY, status);
        case ORDER_STATUS_FILLED_FULLY:
          actualRemainingQty = 0;
          break;
        case ORDER_STATUS_CANCELED:
        case ORDER_STATUS_REJECTED:
        case ORDER_STATUS_ERROR:
        default:
          actualRemainingQty = order.remainingQty;
          break;
      }
    }

    static_assert(numberOfOrderStatuses == 7, "List changed.");
    switch (status) {
      case ORDER_STATUS_SENT:
      case ORDER_STATUS_OPENED:
        switch (order.status) {
          case ORDER_STATUS_FILLED_PARTIALLY:
            m_log.Error(
                "Wrong order status received. Received status \"%1%\", but "
                "it's already is \"%2%\".",
                status,         // 1
                order.status);  // 2
            AssertEq(order.status, status);
            status = order.status;
            break;
        }
        break;
      case ORDER_STATUS_CANCELED:
        if (!order.isCancelRequestSent) {
          static_assert(numberOfTimeInForces == 5, "List changed.");
          switch (order.tif) {
            case TIME_IN_FORCE_DAY:
            case TIME_IN_FORCE_GTC:
            case TIME_IN_FORCE_OPG:
              status = ORDER_STATUS_REJECTED;
              break;
          }
        }
    }

    if (!trade && order.remainingQty != actualRemainingQty) {
      static_assert(numberOfOrderStatuses == 7, "List changed.");
      switch (status) {
        case ORDER_STATUS_CANCELED:
        case ORDER_STATUS_FILLED_FULLY:
        case ORDER_STATUS_FILLED_PARTIALLY:
        case ORDER_STATUS_REJECTED:
          trade = Trade{};
          break;
      }
    } else if (status == ORDER_STATUS_ERROR) {
      trade = boost::none;
    }

    if (trade) {
      if (!trade->price) {
        trade->price = order.actualPrice;
      }
      AssertGe(order.remainingQty, actualRemainingQty);
      const auto tradeQty = order.remainingQty - actualRemainingQty;
      AssertLt(0, tradeQty);
      if (!trade->qty) {
        trade->qty = tradeQty;
      } else if (trade->qty != tradeQty) {
        m_log.Error(
            "Wrong trade quantity received. Received remaining quantity %1$.8f "
            "and trade quantity %2$.8f, remaining quantity  before  update was "
            "%3$.8f.",
            actualRemainingQty,   // 1
            trade->qty,           // 2
            order.remainingQty);  // 3
        status = ORDER_STATUS_ERROR;
        trade = boost::none;
      }
    }

    ReportOrderUpdate(orderId, order, status, actualRemainingQty, commission,
                      trade);

    order.status = remoteStatus;

    {
      const auto orderSafePtr = it->second;
      static_assert(numberOfOrderStatuses == 7, "List changed.");
      switch (status) {
        case ORDER_STATUS_FILLED_FULLY:
        case ORDER_STATUS_CANCELED:
        case ORDER_STATUS_REJECTED:
        case ORDER_STATUS_ERROR:
          m_activeOrders.erase(it);
          lock.unlock();
          Assert(status != ORDER_STATUS_FILLED_FULLY || trade);
          Assert(status != ORDER_STATUS_FILLED_FULLY ||
                 actualRemainingQty == 0);
          Assert(!trade || status != ORDER_STATUS_ERROR);
          ConfirmOrder(*orderSafePtr, status, actualRemainingQty, trade);
          if (commission) {
            orderSafePtr->handler->OnCommission(*commission);
          }
          if (trade) {
            orderSafePtr->handler->OnTrade(*trade,
                                           status == ORDER_STATUS_FILLED_FULLY);
          }
          switch (status) {
            case ORDER_STATUS_CANCELED:
              orderSafePtr->handler->OnCancel();
              break;
            case ORDER_STATUS_REJECTED:
              orderSafePtr->handler->OnReject();
              break;
            case ORDER_STATUS_ERROR:
              orderSafePtr->handler->OnError();
              break;
          }
          break;
        default:
          order.remainingQty = actualRemainingQty;
          order.updateTime = time;
          lock.unlock();
          ConfirmOrder(*orderSafePtr, status, actualRemainingQty, trade);
          if (status == ORDER_STATUS_OPENED) {
            orderSafePtr->handler->OnOpen();
          }
          if (commission) {
            orderSafePtr->handler->OnCommission(*commission);
          }
          if (trade) {
            AssertLt(0, actualRemainingQty);
            orderSafePtr->handler->OnTrade(*trade, false);
          }
          break;
      }
    }

    m_context.InvokeDropCopy([&](DropCopy &dropCopy) {
      dropCopy.CopyOrderStatus(orderId, m_self, time, status,
                               actualRemainingQty);
      if (trade) {
        dropCopy.CopyTrade(time, trade->id, orderId, m_self, trade->price,
                           trade->qty);
      }
    });
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
    virtual boost::optional<Volume> FindAvailableToTrade(
        const std::string &) const override {
      return boost::none;
    }
    virtual void ReduceAvailableToTradeByOrder(const Security &,
                                               const Qty &,
                                               const Price &,
                                               const OrderSide &,
                                               const TradingSystem &) override {
    }

  } result;
  return result;
}

Volume TradingSystem::CalcCommission(const Volume &, const Security &) const {
  return 0;
}

boost::optional<TradingSystem::OrderCheckError> TradingSystem::CheckOrder(
    const Security &,
    const Currency &,
    const Qty &,
    const boost::optional<Price> &,
    const OrderSide &) const {
  return boost::none;
}

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
  auto order = boost::make_shared<Order>(
      Order{security, currency, side, std::move(handler), qty, qty, price,
            price ? *price
                  : side == ORDER_SIDE_BUY ? security.GetAskPrice()
                                           : security.GetBidPrice(),
            ORDER_STATUS_SENT, tif, delayMeasurement, riskControlScope});

  m_pimpl->ReportNewOrder(*order, "sending", price);

  order->riskControlOperationId = m_pimpl->CheckNewOrder(*order);

  try {
    {
      const ActiveOrderWriteLock lock(m_pimpl->m_activeOrdersMutex);
      order->transactionContext =
          SendOrderTransaction(order->security, order->currency, order->qty,
                               price, params, order->side, order->tif);
      Assert(order->transactionContext);
      m_pimpl->ReportNewOrder(*order, ConvertToPch(ORDER_STATUS_SENT), price);
      m_pimpl->RegisterCallback(order);
    }
    OnTransactionSent(*order->transactionContext);
  } catch (const std::exception &ex) {
    GetTradingLog().Write(
        "{'order': {'sendError': {'reason': '%1%'}}}",
        [&ex](TradingRecord &record) { record % std::string(ex.what()); });
    try {
      throw;
    } catch (const CommunicationError &ex) {
      GetLog().Debug(
          "Communication error while sending order transaction: \"%1%\".",
          ex.what());
    } catch (const std::exception &ex) {
      GetLog().Error("Error while sending order transaction: \"%1%\".",
                     ex.what());
    }
    m_pimpl->ConfirmOrder(*order, ORDER_STATUS_ERROR, order->remainingQty,
                          boost::none);
    throw;
  } catch (...) {
    GetTradingLog().Write(
        "{'order': {'sendError': {'reason': 'Unknown exception'}}}");
    GetLog().Error("Unknown error while sending order transaction.");
    AssertFailNoException();
    m_pimpl->ConfirmOrder(*order, ORDER_STATUS_ERROR, order->remainingQty,
                          boost::none);
    throw;
  }

  GetBalancesStorage().ReduceAvailableToTradeByOrder(
      security, order->qty, order->actualPrice, order->side, *this);

  GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {
    dropCopy.CopySubmittedOrder(order->transactionContext->GetOrderId(), time,
                                order->security, order->currency, *this,
                                order->side, order->qty, price, order->tif);
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
          "{'order': {'cancelSendError': {'id': '%1%', 'reason': 'Order is not "
          "in the local active order list.'}}}",
          [&orderId](TradingRecord &record) { record % orderId; });
      return false;
    }
    transaction = it->second->transactionContext;
    if (it->second->isCancelRequestSent) {
      lock.unlock();
      GetTradingLog().Write(
          "{'order': {'cancelSendError': {'id': '%1%', 'reason': 'Order cancel "
          "request is already sent.'}}}",
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

void TradingSystem::OnOrderStatusUpdate(const pt::ptime &time,
                                        const OrderId &orderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty,
                                        Trade &&trade,
                                        const Volume &commission) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, status, remainingQty, commission,
                               std::move(trade),
                               emptyOrderTransactionContextCallback);
}
void TradingSystem::OnOrderStatusUpdate(const pt::ptime &time,
                                        const OrderId &orderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, status, remainingQty, boost::none,
                               boost::none,
                               emptyOrderTransactionContextCallback);
}
void TradingSystem::OnOrderStatusUpdate(const pt::ptime &time,
                                        const OrderId &orderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty,
                                        const Volume &commission) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, status, remainingQty, commission,
                               boost::none,
                               emptyOrderTransactionContextCallback);
}
void TradingSystem::OnOrderStatusUpdate(const pt::ptime &time,
                                        const OrderId &orderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty,
                                        Trade &&trade) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, status, remainingQty, boost::none,
                               std::move(trade),
                               emptyOrderTransactionContextCallback);
}
void TradingSystem::OnOrderStatusUpdate(
    const pt::ptime &time,
    const OrderId &orderId,
    const OrderStatus &status,
    const Qty &remainingQty,
    const boost::function<bool(OrderTransactionContext &)> &pred) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, status, remainingQty, boost::none,
                               boost::none, pred);
}
void TradingSystem::OnOrderStatusUpdate(
    const pt::ptime &time,
    const OrderId &orderId,
    const OrderStatus &status,
    const Qty &remainingQty,
    Trade &&trade,
    const boost::function<bool(OrderTransactionContext &)> &pred) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, status, remainingQty, boost::none,
                               std::move(trade), pred);
}
void TradingSystem::OnOrderStatusUpdate(const pt::ptime &time,
                                        const OrderId &orderId,
                                        const OrderStatus &status) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, status, boost::none, boost::none,
                               boost::none,
                               emptyOrderTransactionContextCallback);
}
void TradingSystem::OnOrderStatusUpdate(const pt::ptime &time,
                                        const OrderId &orderId,
                                        const OrderStatus &status,
                                        Trade &&trade) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, status, boost::none, boost::none,
                               std::move(trade),
                               emptyOrderTransactionContextCallback);
}
void TradingSystem::OnOrderStatusUpdate(
    const pt::ptime &time,
    const OrderId &orderId,
    const OrderStatus &status,
    Trade &&trade,
    const boost::function<bool(OrderTransactionContext &)> &pred) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, status, boost::none, boost::none,
                               std::move(trade), pred);
}
void TradingSystem::OnOrderStatusUpdate(const pt::ptime &time,
                                        const OrderId &orderId,
                                        const OrderStatus &status,
                                        Trade &&trade,
                                        const Volume &commission) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, status, boost::none, commission,
                               std::move(trade),
                               emptyOrderTransactionContextCallback);
}

void TradingSystem::OnOrderCancel(const pt::ptime &time,
                                  const OrderId &orderId) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, ORDER_STATUS_CANCELED,
                               boost::none, boost::none, boost::none,
                               emptyOrderTransactionContextCallback);
}
void TradingSystem::OnOrderCancel(const pt::ptime &time,
                                  const OrderId &orderId,
                                  const Qty &remainingQty) {
  m_pimpl->OnOrderStatusUpdate(time, orderId, ORDER_STATUS_CANCELED,
                               remainingQty, boost::none, boost::none,
                               emptyOrderTransactionContextCallback);
}

void TradingSystem::OnOrderError(const pt::ptime &time,
                                 const OrderId &orderId,
                                 const std::string &&error) {
  GetTradingLog().Write("{'order': {'error': {'id': '%1%', 'reason': '%2%'}}}",
                        [&](TradingRecord &record) {
                          record % orderId  // 1
                              % error;      // 2
                        });
  GetLog().Warn(
      "Operation request for the order %1% is rejected with the reason: "
      "\"%2%\".",
      orderId,  // 1
      error);   // 2
  m_pimpl->OnOrderStatusUpdate(time, orderId, ORDER_STATUS_ERROR, boost::none,
                               boost::none, boost::none,
                               emptyOrderTransactionContextCallback);
}

void TradingSystem::OnOrderReject(const pt::ptime &time,
                                  const OrderId &orderId,
                                  const std::string &&reason) {
  GetTradingLog().Write("{'order': {'reject': {'id': '%1%', 'reason': '%2%'}}}",
                        [&](TradingRecord &record) {
                          record % orderId  // 1
                              % reason;     // 2
                        });
  m_pimpl->OnOrderStatusUpdate(time, orderId, ORDER_STATUS_REJECTED,
                               boost::none, boost::none, boost::none,
                               emptyOrderTransactionContextCallback);
}

void TradingSystem::OnOrder(const OrderId &orderId,
                            const std::string &symbol,
                            const OrderStatus &status,
                            const Qty &qty,
                            const Qty &remainingQty,
                            const boost::optional<Price> &price,
                            const OrderSide &side,
                            const TimeInForce &tif,
                            const pt::ptime &openTime,
                            const pt::ptime &updateTime) {
  {
    ActiveOrderWriteLock lock(m_pimpl->m_activeOrdersMutex);
    const auto &it = m_pimpl->m_activeOrders.find(orderId);
    if (it != m_pimpl->m_activeOrders.cend()) {
      m_pimpl->OnOrderStatusUpdate(
          updateTime, orderId, status, remainingQty, boost::none, boost::none,
          it, std::move(lock), emptyOrderTransactionContextCallback);
      return;
    }
  }
  GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {
    dropCopy.CopyOrder(orderId, *this, symbol, status, qty, remainingQty, price,
                       side, tif, openTime, updateTime);
  });
}
