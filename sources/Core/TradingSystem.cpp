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
#include "DropCopy.hpp"
#include "RiskControl.hpp"
#include "Security.hpp"
#include "Timer.hpp"
#include "TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;

namespace pt = boost::posix_time;

//////////////////////////////////////////////////////////////////////////

TradingSystem::Error::Error(const char *what) noexcept : Base::Error(what) {}

TradingSystem::OrderParamsError::OrderParamsError(const char *what,
                                                  const Qty &,
                                                  const OrderParams &) noexcept
    : Error(what) {}

TradingSystem::OrderIsUnknown::OrderIsUnknown(const char *what) noexcept
    : Error(what) {}

TradingSystem::ConnectionDoesntExistError::ConnectionDoesntExistError(
    const char *what) noexcept
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

template <trdk::Lib::Concurrency::Profile profile>
struct ActiveOrderConcurrencyPolicy {
  static_assert(profile == trdk::Lib::Concurrency::PROFILE_RELAX,
                "Wrong concurrency profile");
  typedef boost::mutex Mutex;
  typedef Mutex::scoped_lock Lock;
};
template <>
struct ActiveOrderConcurrencyPolicy<trdk::Lib::Concurrency::PROFILE_HFT> {
  typedef trdk::Lib::Concurrency::SpinMutex Mutex;
  typedef Mutex::ScopedLock Lock;
};
typedef ActiveOrderConcurrencyPolicy<TRDK_CONCURRENCY_PROFILE>::Mutex
    ActiveOrderMutex;
typedef ActiveOrderConcurrencyPolicy<TRDK_CONCURRENCY_PROFILE>::Lock
    ActiveOrderLock;

struct ActiveOrder {
  TradingSystem::OrderStatusUpdateSlot statusUpdateSignal;
  Qty remainingQty;
  OrderStatus status;
  std::unique_ptr<trdk::Timer::Scope> timerScope;
};
}

class TradingSystem::Implementation : private boost::noncopyable {
 public:
  TradingSystem &m_self;

  const TradingMode m_mode;

  const size_t m_index;

  Context &m_context;

  const std::string m_instanceName;
  const std::string m_stringId;

  TradingSystem::Log m_log;
  TradingSystem::TradingLog m_tradingLog;

  boost::atomic_size_t m_lastOperationId;

  ActiveOrderMutex m_activeOrdersMutex;
  boost::unordered_map<OrderId, ActiveOrder> m_activeOrders;
  std::unique_ptr<Timer::Scope> m_lastOrderTimerScope;

  explicit Implementation(TradingSystem &self,
                          const TradingMode &mode,
                          size_t index,
                          Context &context,
                          const std::string &instanceName)
      : m_self(self),
        m_mode(mode),
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

  void LogNewOrder(size_t operationId,
                   Security &security,
                   const Currency &currency,
                   const Qty &qty,
                   const boost::optional<Price> &orderPrice,
                   const Price &actualPrice,
                   const OrderSide &side,
                   const TimeInForce &tif) {
    m_tradingLog.Write(
        "{'newOrder': {'operationId': %1%, 'side': '%2%', 'security': '%3%', "
        "'currency': '%4%', 'type': '%5%', 'price': %6$.8f, 'qty': %7$.8f"
        ", 'tif': '%8%'}}",
        [&](TradingRecord &record) {
          record % operationId                                       // 1
              % side                                                 // 2
              % security                                             // 3
              % currency                                             // 4
              % (orderPrice ? ORDER_TYPE_LIMIT : ORDER_TYPE_MARKET)  // 5
              % actualPrice                                          // 6
              % qty                                                  // 7
              % tif;                                                 // 8
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
        "{'orderStatus': {'status': '%1%', 'remainingQty': %2$.8f, "
        "'operationId': %3%, 'id': %4%, 'tsId': '%5%', 'commission': %6$.8f}}",
        [&](TradingRecord &record) {
          record % orderStatus         // 1
              % remainingQty           // 2
              % operationId            // 3
              % orderId                // 4
              % tradingSystemOrderId;  // 5
          if (commission) {
            record % *commission;
          } else {
            record % "NaN";
          }
        });
    if (trade) {
      m_tradingLog.Write(
          "{'trade': {'id': '%1%', 'qty': %2$.8f, 'price': %3$.8f}}",
          [&trade](TradingRecord &record) {
            if (trade->id) {
              record % *trade->id;  // 1
            } else {
              record % "";  // 1
            }
            record % trade->qty  // 2
                % trade->price;  // 3
          });
    }
  }

  RiskControlOperationId CheckNewBuyOrder(RiskControlScope &riskControlScope,
                                          Security &security,
                                          const Currency &currency,
                                          const Qty &qty,
                                          const Price &price,
                                          const Milestones &delaysMeasurement) {
    return m_context.GetRiskControl(m_mode).CheckNewBuyOrder(
        riskControlScope, security, currency, qty, price, delaysMeasurement);
  }
  RiskControlOperationId CheckNewSellOrder(RiskControlScope &riskControlScope,
                                           Security &security,
                                           const Currency &currency,
                                           const Qty &qty,
                                           const Price &price,
                                           const Milestones &timeMeasurement) {
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
                       const Milestones &timeMeasurement) {
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
                        const Milestones &timeMeasurement) {
    m_context.GetRiskControl(m_mode).ConfirmSellOrder(
        operationId, riskControlScope, orderStatus, security, currency,
        orderPrice, remainingQty, trade, timeMeasurement);
  }

  void RegisterCallback(const OrderId &orderId,
                        const OrderStatusUpdateSlot &&callback,
                        const Qty &qty) {
    auto result = m_activeOrders.emplace(
        orderId, ActiveOrder{std::move(callback), qty, ORDER_STATUS_SENT});
    if (!result.second) {
      m_log.Error("Order ID %1% is not unique.", orderId);
      throw Error("Order ID %1% is not unique");
    }
    Assert(!result.first->second.timerScope);
    if (m_lastOrderTimerScope) {
      result.first->second.timerScope = std::move(m_lastOrderTimerScope);
    }
  }

  void OnOrderStatusUpdate(const OrderId &orderId,
                           const std::string &tradingSystemOrderId,
                           const OrderStatus &status,
                           const boost::optional<Qty> &remainingQty,
                           const boost::optional<Volume> &commission,
                           boost::optional<TradeInfo> &&tradeInfo) {
    const auto &time = m_context.GetCurrentTime();
    const ActiveOrderLock lock(m_activeOrdersMutex);
    const auto &it = m_activeOrders.find(orderId);
    if (it == m_activeOrders.cend()) {
      m_log.Warn(
          "Failed to handle status update for order %1% as order is unknown.",
          orderId);
      throw OrderIsUnknown(
          "Failed to handle status update for order as order is unknown");
    }
    OnOrderStatusUpdate(orderId, tradingSystemOrderId, status, remainingQty,
                        commission, std::move(tradeInfo), time, it);
  }

  void OnOrderStatusUpdate(
      const OrderId &orderId,
      const std::string &tradingSystemOrderId,
      OrderStatus status,
      const boost::optional<Qty> &remainingQty,
      const boost::optional<Volume> &commission,
      boost::optional<TradeInfo> &&tradeInfo,
      const pt::ptime &time,
      const boost::unordered_map<OrderId, ActiveOrder>::iterator &it) {
    static_assert(numberOfOrderStatuses == 8, "List changed.");
    switch (status) {
      case ORDER_STATUS_SENT:
      case ORDER_STATUS_SUBMITTED:
        switch (it->second.status) {
          case ORDER_STATUS_FILLED:
          case ORDER_STATUS_FILLED_PARTIALLY:
            status = it->second.status;
            break;
          default:
            it->second.status = status;
            break;
        }
        break;
      default:
        it->second.status = status;
        break;
    }

    if (remainingQty) {
      AssertGe(it->second.remainingQty, *remainingQty);
      if (tradeInfo && !tradeInfo->qty) {
        tradeInfo->qty = it->second.remainingQty - *remainingQty;
      }
      it->second.remainingQty = *remainingQty;
    }

    // Maybe in the future new thread will be required here to avoid deadlocks
    // from callback (when some one will need to call the same trading system
    // from this callback).
    it->second.statusUpdateSignal(orderId, tradingSystemOrderId, status,
                                  it->second.remainingQty, commission,
                                  tradeInfo ? &*tradeInfo : nullptr);

    m_context.InvokeDropCopy([&](DropCopy &dropCopy) {
      dropCopy.CopyOrderStatus(orderId, tradingSystemOrderId, m_self, time,
                               status, it->second.remainingQty);
      if (tradeInfo) {
        dropCopy.CopyTrade(time, tradeInfo->id, orderId, m_self,
                           tradeInfo->price, tradeInfo->qty);
      }
    });

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
          *this, mode, index, context, instanceName)) {}

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

std::vector<OrderId> TradingSystem::GetActiveOrderList() const {
  std::vector<OrderId> result;
  {
    const ActiveOrderLock lock(m_pimpl->m_activeOrdersMutex);
    result.reserve(m_pimpl->m_activeOrders.size());
    for (const auto &order : m_pimpl->m_activeOrders) {
      result.emplace_back(order.first);
    }
  }
  return result;
}

void TradingSystem::Connect(const IniSectionRef &conf) {
  if (IsConnected()) {
    return;
  }
  CreateConnection(conf);
}

OrderId TradingSystem::SendOrder(Security &security,
                                 const Currency &currency,
                                 const Qty &qty,
                                 const boost::optional<Price> &price,
                                 const OrderParams &params,
                                 const OrderStatusUpdateSlot &callback,
                                 RiskControlScope &riskControlScope,
                                 const OrderSide &side,
                                 const TimeInForce &tif,
                                 const Milestones &delaysMeasurement) {
  const auto operationId = ++m_pimpl->m_lastOperationId;
  const Price &actualPrice = price ? *price
                                   : side == ORDER_SIDE_BUY
                                         ? security.GetAskPrice()
                                         : security.GetBidPrice();
  m_pimpl->LogNewOrder(operationId, security, currency, qty, price, actualPrice,
                       side, tif);

  const auto riskControlOperationId =
      side == ORDER_SIDE_BUY
          ? m_pimpl->CheckNewBuyOrder(riskControlScope, security, currency, qty,
                                      actualPrice, delaysMeasurement)
          : m_pimpl->CheckNewSellOrder(riskControlScope, security, currency,
                                       qty, actualPrice, delaysMeasurement);

  try {
    return SendOrderTransaction(
        security, currency, qty, price, params, side, tif,
        [this, operationId, &riskControlScope, riskControlOperationId,
         &security, currency, actualPrice, delaysMeasurement, callback, side](
            const OrderId &orderId, const std::string &tradingSystemOrderId,
            const OrderStatus &orderStatus, const Qty &remainingQty,
            const boost::optional<Volume> &commission, const TradeInfo *trade) {
          m_pimpl->LogOrderUpdate(operationId, orderId, tradingSystemOrderId,
                                  orderStatus, remainingQty, commission, trade);
          side == ORDER_SIDE_BUY
              ? m_pimpl->ConfirmBuyOrder(riskControlOperationId,
                                         riskControlScope, orderStatus,
                                         security, currency, actualPrice,
                                         remainingQty, trade, delaysMeasurement)
              : m_pimpl->ConfirmSellOrder(
                    riskControlOperationId, riskControlScope, orderStatus,
                    security, currency, actualPrice, remainingQty, trade,
                    delaysMeasurement);
          callback(orderId, tradingSystemOrderId, orderStatus, remainingQty,
                   commission, trade);
        });
  } catch (const std::exception &ex) {
    GetTradingLog().Write(
        "{'orderSendError': {'reason': '%1%', 'operationId': %2%}}",
        [&ex, &operationId](TradingRecord &record) {
          record % ex.what()  // 1
              % operationId;  // 2
        });
    GetLog().Warn("Error while sending order transaction: \"%1%\".", ex.what());
    m_pimpl->ConfirmSellOrder(riskControlOperationId, riskControlScope,
                              ORDER_STATUS_ERROR, security, currency,
                              actualPrice, qty, nullptr, delaysMeasurement);
    throw;
  } catch (...) {
    GetTradingLog().Write(
        "{'orderSendError': {'reason': 'Unknown exception', 'operationId': "
        "%1%}}",
        [&operationId](TradingRecord &record) {
          record % operationId;  // 1
        });
    GetLog().Error("Unknown error while sending order transaction.");
    AssertFailNoException();
    m_pimpl->ConfirmSellOrder(riskControlOperationId, riskControlScope,
                              ORDER_STATUS_ERROR, security, currency,
                              actualPrice, qty, nullptr, delaysMeasurement);
    throw;
  }
}

OrderId TradingSystem::SendOrderTransaction(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderParams &params,
    const OrderSide &side,
    const TimeInForce &tif,
    const OrderStatusUpdateSlot &&callback) {
  const ActiveOrderLock lock(m_pimpl->m_activeOrdersMutex);
  const auto &time = GetContext().GetCurrentTime();
  const auto &orderId =
      SendOrderTransaction(security, currency, qty, price, params, side, tif);
  m_pimpl->RegisterCallback(orderId, std::move(callback), qty);
  GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {
    dropCopy.CopySubmittedOrder(orderId, time, security, currency, *this, side,
                                qty, price, tif);
  });
  return orderId;
}

OrderId TradingSystem::SendOrderTransactionAndEmulateIoc(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderParams &params,
    const OrderSide &side) {
  const auto &orderId = SendOrderTransaction(security, currency, qty, price,
                                             params, side, TIME_IN_FORCE_GTC);
  Assert(!m_pimpl->m_lastOrderTimerScope);
  m_pimpl->m_lastOrderTimerScope = boost::make_unique<Timer::Scope>(orderId);
  GetContext().GetTimer().Schedule(
      params.goodInTime ? *params.goodInTime : pt::milliseconds(300),
      [this, orderId] { CancelOrder(orderId); },
      *m_pimpl->m_lastOrderTimerScope);
  return orderId;
}

void TradingSystem::CancelOrder(const OrderId &orderId) {
  GetTradingLog().Write(
      "{'orderCancel': {'id': %1%}}",
      [&orderId](TradingRecord &record) { record % orderId; });
  try {
    SendCancelOrderTransaction(orderId);
  } catch (const std::exception &ex) {
    GetTradingLog().Write(
        "{'orderCancelSendError': {'id': %1%, 'reason': '%2%'}}",
        [&orderId, &ex](TradingRecord &record) {
          record % orderId  // 1
              % ex.what();  // 2
        });
    GetLog().Warn(
        "Error while sending order cancel transaction for order %1%: \"%2%\".",
        orderId,     // 1
        ex.what());  // 2
    throw;
  } catch (...) {
    GetTradingLog().Write(
        "{'orderCancelSendError': {'id': %1%, 'reason': 'Unknown exception'}}",
        [&orderId](TradingRecord &record) { record % orderId; });
    GetLog().Error(
        "Unknown error while sending order cancel transaction for order %1%.",
        orderId);
    AssertFailNoException();
    throw;
  }
}

void TradingSystem::OnSettingsUpdate(const IniSectionRef &) {}

void TradingSystem::OnOrderStatusUpdate(const OrderId &orderId,
                                        const std::string &tradingSystemOrderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty,
                                        const Volume &commission,
                                        TradeInfo &&trade) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId, status,
                               remainingQty, std::move(commission),
                               std::move(trade));
}
void TradingSystem::OnOrderStatusUpdate(const OrderId &orderId,
                                        const std::string &tradingSystemOrderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId, status,
                               remainingQty, boost::none, boost::none);
}
void TradingSystem::OnOrderStatusUpdate(const OrderId &orderId,
                                        const std::string &tradingSystemOrderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty,
                                        const Volume &commission) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId, status,
                               remainingQty, commission, boost::none);
}
void TradingSystem::OnOrderStatusUpdate(const OrderId &orderId,
                                        const std::string &tradingSystemOrderId,
                                        const OrderStatus &status,
                                        const Qty &remainingQty,
                                        TradeInfo &&trade) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId, status,
                               remainingQty, boost::none, std::move(trade));
}

void TradingSystem::OnOrderCancel(const OrderId &orderId,
                                  const std::string &tradingSystemOrderId) {
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId,
                               ORDER_STATUS_CANCELLED, boost::none, boost::none,
                               boost::none);
}

void TradingSystem::OnOrderError(const OrderId &orderId,
                                 const std::string &tradingSystemOrderId,
                                 const std::string &&error) {
  GetTradingLog().Write("{'orderError': {'id': %1%, 'reason': '%2%'}}",
                        [&](TradingRecord &record) {
                          record % orderId  // 1
                              % error;      // 2
                        });
  GetLog().Warn("Order %1% is rejected with the reason: \"%2%\".", orderId,
                error);
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId,
                               ORDER_STATUS_ERROR, boost::none, boost::none,
                               boost::none);
}

void TradingSystem::OnOrderReject(const OrderId &orderId,
                                  const std::string &tradingSystemOrderId,
                                  const std::string &&reason) {
  GetTradingLog().Write("{'orderReject': {'id': %1%, 'reason': '%2%'}}",
                        [&](TradingRecord &record) {
                          record % orderId  // 1
                              % reason;     // 2
                        });
  m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId,
                               ORDER_STATUS_REJECTED, boost::none, boost::none,
                               boost::none);
}

void TradingSystem::OnOrder(const OrderId &orderId,
                            const std::string &tradingSystemOrderId,
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
    const ActiveOrderLock lock(m_pimpl->m_activeOrdersMutex);
    const auto &it = m_pimpl->m_activeOrders.find(orderId);
    if (it != m_pimpl->m_activeOrders.cend()) {
      m_pimpl->OnOrderStatusUpdate(orderId, tradingSystemOrderId, status,
                                   remainingQty, boost::none, boost::none,
                                   updateTime, it);
      return;
    }
  }
  GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {
    dropCopy.CopyOrder(orderId, tradingSystemOrderId, *this, symbol, status,
                       qty, remainingQty, price, side, tif, openTime,
                       updateTime);
  });
}

////////////////////////////////////////////////////////////////////////////////

LegacyTradingSystem::LegacyTradingSystem(const TradingMode &tradingMode,
                                         size_t index,
                                         Context &context,
                                         const std::string &instanceName)
    : TradingSystem(tradingMode, index, context, instanceName) {}

OrderId LegacyTradingSystem::SendOrderTransaction(
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const boost::optional<Price> &price,
    const OrderParams &params,
    const OrderSide &side,
    const TimeInForce &tif,
    const OrderStatusUpdateSlot &&callback) {
  if (side == ORDER_SIDE_BUY) {
    if (price) {
      switch (tif) {
        case TIME_IN_FORCE_GTC:
          return SendBuy(security, currency, qty, *price, params,
                         std::move(callback));
        case TIME_IN_FORCE_IOC:
          return SendBuyImmediatelyOrCancel(security, currency, qty, *price,
                                            params, std::move(callback));
      }
    } else {
      switch (tif) {
        case TIME_IN_FORCE_GTC:
          return SendBuyAtMarketPrice(security, currency, qty, params,
                                      std::move(callback));
      }
    }
  } else {
    if (price) {
      switch (tif) {
        case TIME_IN_FORCE_GTC:
          return SendSell(security, currency, qty, *price, params,
                          std::move(callback));
        case TIME_IN_FORCE_IOC:
          return SendSellImmediatelyOrCancel(security, currency, qty, *price,
                                             params, std::move(callback));
      }
    } else {
      switch (tif) {
        case TIME_IN_FORCE_GTC:
          return SendSellAtMarketPrice(security, currency, qty, params,
                                       std::move(callback));
      }
    }
  }
  AssertFail("Legacy trading system can not support all order types.");
  throw Error("Unsupported order type");
}

OrderId LegacyTradingSystem::SendOrderTransaction(
    Security &,
    const Currency &,
    const Qty &,
    const boost::optional<Price> &,
    const OrderParams &,
    const OrderSide &,
    const TimeInForce &) {
  AssertFail(
      "Internal error of legacy trading system implementation: this method "
      "never should be called, as legacy trading system uses own methods "
      "with "
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
