/**************************************************************************
 *   Created: 2012/07/08 04:07:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Position.hpp"
#include "TradingLib/Algo.hpp"
#include "PositionOperationContext.hpp"
#include "Settings.hpp"
#include "Strategy.hpp"
#include "TradingLog.hpp"
#include "TransactionContext.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

namespace pt = boost::posix_time;
namespace uuids = boost::uuids;
namespace sig = boost::signals2;

//////////////////////////////////////////////////////////////////////////

Position::Exception::Exception(const char *what) noexcept
    : Lib::Exception(what) {}

Position::AlreadyStartedError::AlreadyStartedError() noexcept
    : Exception("Position already started") {}

Position::NotOpenedError::NotOpenedError() noexcept
    : Exception("Position not opened") {}

Position::AlreadyClosedError::AlreadyClosedError() noexcept
    : Exception("Position already closed") {}

//////////////////////////////////////////////////////////////////////////

class Position::Implementation : private boost::noncopyable {
 public:
  template <typename SlotSignature>
  struct SignalTrait {
    typedef sig::signal<
        SlotSignature,
        sig::optional_last_value<
            typename boost::function_traits<SlotSignature>::result_type>,
        int,
        std::less<int>,
        boost::function<SlotSignature>,
        typename sig::detail::extended_signature<
            boost::function_traits<SlotSignature>::arity,
            SlotSignature>::function_type,
        sig::dummy_mutex>
        Signal;
  };
  typedef SignalTrait<StateUpdateSlotSignature>::Signal StateUpdateSignal;

  struct Order {
    const pt::ptime time;

    bool isActive;
    bool isCanceled;

    const boost::optional<Price> price;
    const Qty qty;

    boost::shared_ptr<const TransactionContext> transactionContext;

    Qty executedQty;

    Volume commission;

    explicit Order(const pt::ptime &&time,
                   boost::optional<Price> &&price,
                   const Qty &qty)
        : time(std::move(time)),
          isActive(true),
          isCanceled(false),
          price(std::move(price)),
          qty(qty),
          executedQty(0),
          commission(0) {}
  };

  struct CloseOrder : public Order {
    explicit CloseOrder(const pt::ptime &&time,
                        boost::optional<Price> &&price,
                        const Qty &qty)
        : Order(std::move(time), std::move(price), qty) {}
  };

  template <typename Order>
  struct DirectionData {
    Price startPrice;

    pt::ptime time;

    Volume volume;
    Qty qty;
    size_t numberOfTrades;
    Price lastTradePrice;
    std::vector<Order> orders;

    explicit DirectionData(const Price &startPrice)
        : startPrice(startPrice),
          volume(0),
          qty(0),
          numberOfTrades(0),
          lastTradePrice(0) {}

    bool HasActiveOrders() const {
      return !orders.empty() && orders.back().isActive;
    }

    bool IsCanceling() const {
      return HasActiveOrders() && orders.back().isCanceled;
    }

    void OnNewTrade(const TradingSystem::TradeInfo &trade, const Price &price) {
      AssertLt(0, trade.price);
      AssertLt(0, trade.qty);
      Assert(!orders.empty());
      auto &order = orders.back();
      order.executedQty += trade.qty;
      volume += price * trade.qty;
      qty += trade.qty;
      ++numberOfTrades;
      lastTradePrice = trade.price;
    }
  };

  typedef DirectionData<Order> OpenData;
  typedef DirectionData<CloseOrder> CloseData;

 public:
  Position &m_self;

  const boost::shared_ptr<PositionOperationContext> m_operationContext;

  TradingSystem &m_tradingSystem;

  mutable StateUpdateSignal m_stateUpdateSignal;

  Strategy &m_strategy;
  const uuids::uuid m_operationId;
  const int64_t m_subOperationId;
  bool m_isRegistered;
  Security &m_security;
  const Currency m_currency;

  Qty m_planedQty;

  boost::optional<ContractExpiration> m_expiration;

  bool m_isMarketAsCompleted;

  bool m_isError;
  bool m_isInactive;

  TimeMeasurement::Milestones m_timeMeasurement;

  OpenData m_open;
  CloseData m_close;

  CloseReason m_closeReason;

  std::vector<boost::shared_ptr<Algo>> m_algos;

  OrderParams m_defaultOrderParams;

  explicit Implementation(
      Position &position,
      const boost::shared_ptr<PositionOperationContext> &operationContext,
      TradingSystem &tradingSystem,
      Strategy &strategy,
      const uuids::uuid &operationId,
      int64_t subOperationId,
      Security &security,
      const Currency &currency,
      const Qty &qty,
      const Price &startPrice,
      const TimeMeasurement::Milestones &timeMeasurement)
      : m_self(position),
        m_operationContext(operationContext),
        m_tradingSystem(tradingSystem),
        m_strategy(strategy),
        m_operationId(operationId),
        m_subOperationId(subOperationId),
        m_isRegistered(false),
        m_security(security),
        m_currency(currency),
        m_planedQty(qty),
        m_isMarketAsCompleted(false),
        m_isError(false),
        m_isInactive(false),
        m_timeMeasurement(timeMeasurement),
        m_open(startPrice),
        m_close(0),
        m_closeReason(CLOSE_REASON_NONE) {
    AssertLt(0, m_planedQty);
  }

 public:
  void UpdateOpening(const OrderId &orderId,
                     const std::string &tradingSystemOrderId,
                     const OrderStatus &orderStatus,
                     const Qty &remainingQty,
                     const boost::optional<Volume> &commission,
                     const TradingSystem::TradeInfo *trade) {
    auto lock = m_strategy.LockForOtherThreads();

    Assert(!m_self.IsOpened());
    Assert(!m_self.IsClosed());
    Assert(!m_self.IsCompleted());
    Assert(!m_open.orders.empty());
    Assert(m_close.orders.empty());
    Assert(m_close.time.is_not_a_date_time());

    Order &order = m_open.orders.back();
    AssertGe(order.qty, remainingQty);
    AssertLe(order.qty, m_planedQty);
    Assert(order.isActive);
    if (!order.transactionContext ||
        order.transactionContext->GetOrderId() != orderId) {
      Assert(order.transactionContext);
      AssertEq(order.transactionContext->GetOrderId(), orderId);
      throw Exception("Unknown open order ID");
    }

    if (commission) {
      order.commission = *commission;
    }

    static_assert(numberOfOrderStatuses == 8, "List changed.");
    switch (orderStatus) {
      default:
        AssertFail("Unknown open order status");
        throw Exception("Unknown open order status");
      case ORDER_STATUS_SENT:
      case ORDER_STATUS_REQUESTED_CANCEL:
        AssertFail("Status can be set only by this object.");
        throw Exception("Status can be set only by this object");
      case ORDER_STATUS_SUBMITTED:
        AssertEq(0, order.executedQty);
        AssertLt(0, remainingQty);
        AssertEq(remainingQty, order.qty);
        Assert(!trade);
        ReportOpeningUpdate(tradingSystemOrderId, orderStatus);
        return;
      case ORDER_STATUS_FILLED:
      case ORDER_STATUS_FILLED_PARTIALLY:
        Assert(trade);
        if (!trade) {
          throw Exception("Filled order has no trade information");
        }
        AssertEq(order.executedQty + trade->qty + remainingQty, order.qty);
        m_open.OnNewTrade(*trade, trade->price);
        if (!m_defaultOrderParams.position) {
          m_defaultOrderParams.position = &*order.transactionContext;
        }
        ReportOpeningUpdate(tradingSystemOrderId, orderStatus);
        order.isActive = remainingQty > 0;
        break;
      case ORDER_STATUS_ERROR:
        order.isActive = false;
        ReportOpeningUpdate(tradingSystemOrderId, orderStatus);
        m_isError = true;
        break;
      case ORDER_STATUS_CANCELLED:
      case ORDER_STATUS_REJECTED:
        order.isActive = false;
        ReportOpeningUpdate(tradingSystemOrderId, orderStatus);
        break;
    }

    AssertEq(order.isActive, m_open.HasActiveOrders());
    if (!order.isActive) {
      AssertLe(order.executedQty, order.qty);

      if (order.qty && m_open.time.is_not_a_date_time()) {
        m_open.time = m_security.GetContext().GetCurrentTime();
      }
    }

    lock.unlock();

    if (!order.isActive) {
      SignalUpdate();
    }
  }

  void UpdateClosing(const OrderId &orderId,
                     const std::string &tradingSystemOrderId,
                     const OrderStatus &orderStatus,
                     const Qty &remainingQty,
                     const boost::optional<Volume> &commission,
                     const TradingSystem::TradeInfo *trade) {
    auto lock = m_strategy.LockForOtherThreads();

    Assert(m_self.IsOpened());
    Assert(!m_self.IsClosed());
    Assert(!m_self.IsCompleted());
    Assert(!m_open.time.is_not_a_date_time());
    Assert(m_close.time.is_not_a_date_time());
    Assert(!m_open.orders.empty());
    Assert(!m_open.HasActiveOrders());
    Assert(!m_close.orders.empty());

    CloseOrder &order = m_close.orders.back();
    AssertLt(0, order.qty);
    AssertGe(order.qty, remainingQty);
    AssertLe(order.executedQty, order.qty);
    Assert(order.isActive);
    if (!order.transactionContext ||
        order.transactionContext->GetOrderId() != orderId) {
      Assert(order.transactionContext);
      AssertEq(order.transactionContext->GetOrderId(), orderId);
      throw Exception("Unknown open order ID");
    }

    if (commission) {
      order.commission = *commission;
    }

    static_assert(numberOfOrderStatuses == 8, "List changed.");
    switch (orderStatus) {
      default:
        AssertFail("Unknown order status.");
        throw Exception("Unknown close order status");
      case ORDER_STATUS_SENT:
      case ORDER_STATUS_REQUESTED_CANCEL:
        AssertFail("Status can be set only by this object.");
        throw Exception("Status can be set only by this object");
      case ORDER_STATUS_SUBMITTED:
        AssertEq(0, order.executedQty);
        AssertLt(0, remainingQty);
        AssertEq(remainingQty, order.qty);
        Assert(!trade);
        ReportClosingUpdate(tradingSystemOrderId, orderStatus);
        return;
      case ORDER_STATUS_FILLED:
      case ORDER_STATUS_FILLED_PARTIALLY:
        Assert(trade);
        if (!trade) {
          throw Exception("Filled order has no trade information");
        }
        AssertEq(order.executedQty + trade->qty + remainingQty, order.qty);
        m_close.OnNewTrade(*trade, trade->price);
        ReportClosingUpdate(tradingSystemOrderId, orderStatus);
        if (remainingQty != 0) {
          return;
        }
        break;
      case ORDER_STATUS_ERROR:
        ReportClosingUpdate(tradingSystemOrderId, orderStatus);
        m_isError = true;
        break;
      case ORDER_STATUS_CANCELLED:
      case ORDER_STATUS_REJECTED:
        ReportClosingUpdate(tradingSystemOrderId, orderStatus);
        break;
    }

    AssertLe(order.executedQty, order.qty);
    Assert(m_close.time.is_not_a_date_time());

    if (!m_self.GetActiveQty()) {
      Assert(m_close.time.is_not_a_date_time());
      m_close.time = m_security.GetContext().GetCurrentTime();
    }

    order.isActive = false;
    AssertEq(order.isActive, m_close.HasActiveOrders());

    lock.unlock();

    SignalUpdate();
  }

  void SignalUpdate() { m_stateUpdateSignal(); }

 public:
  void ReportOpeningStateRestpration() const noexcept {
    Assert(!m_open.orders.empty());
    m_strategy.GetTradingLog().Write(
        "order\trestored\t%1%\t%2%\t%3%.%4%"
        "\tprice=%5$.8f->%6$.8f\t%7%\tqty=%8$.8f\tpos=%9%",
        [this](TradingRecord &record) {
          const auto &order = m_open.orders.back();
          record % m_self.GetOpenOrderSide()                         // 1
              % m_security.GetSymbol().GetSymbol().c_str()           // 2
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 3
              % m_tradingSystem.GetMode()                            // 4
              % m_open.startPrice;                                   // 5
          if (order.price) {
            record % *order.price;  // 6
          } else {
            record % '-';  // 6
          }
          record % m_self.GetCurrency()  // 7
              % m_self.GetPlanedQty()    // 8
              % m_operationId;           // 9
        });
  }

  void ReportOpeningStart(const char *eventDesc,
                          const boost::optional<OrderId> &id) const noexcept {
    Assert(!m_open.orders.empty());
    m_strategy.GetTradingLog().Write(
        "order\topen->%1%\t%2%\t%3%\t%4%.%5%"
        "\tprice=%6$.8f->%7$.8f\t%8%\tqty=%9$.8f"
        "\tbid/ask=%10$.8f/%11$.8f"
        "\tpos=%12%\torder=%13%/-",
        [this, eventDesc, &id](TradingRecord &record) {
          const auto &order = m_open.orders.back();
          record % eventDesc                                         // 1
              % m_self.GetOpenOrderSide()                            // 2
              % m_security.GetSymbol().GetSymbol().c_str()           // 3
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 4
              % m_tradingSystem.GetMode()                            // 5
              % m_open.startPrice;                                   // 6
          if (order.price) {
            record % *order.price;  // 7
          } else {
            record % '-';  // 7
          }
          record % m_self.GetCurrency()        // 8
              % m_self.GetPlanedQty()          // 9
              % m_security.GetBidPriceValue()  // 10
              % m_security.GetAskPriceValue()  // 11
              % m_operationId;                 // 12
          if (id) {
            record % *id;  // 13 and last
          } else {
            record % '-';  // 13 and last
          }
        });
  }

  void ReportOpeningUpdate(const std::string &tsOrderId,
                           const OrderStatus &orderStatus) const noexcept {
    Assert(!m_open.orders.empty());
    m_strategy.GetTradingLog().Write(
        "order\topen->%1%\t%2%\t%3%"
        "\t%4%.%5%\tprice=%6$.8f->%7$.8f->%8$.8f(avg=%9$.8f)\t%10%"
        "\tqty=%11$.8f->%12$.8f\tbid/ask=%13$.8f/%14$.8f"
        "\tpos=%15%\torder=%16%/%17%",
        [this, &tsOrderId, &orderStatus](TradingRecord &record) {
          const auto &order = m_open.orders.back();
          record % orderStatus                                       // 1
              % m_self.GetOpenOrderSide()                            // 2
              % m_security.GetSymbol().GetSymbol().c_str()           // 3
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 4
              % m_tradingSystem.GetMode()                            // 5
              % m_self.GetOpenStartPrice();                          // 6
          if (order.price) {
            record % *order.price;  // 7
          } else {
            record % '-';  // 7
          }
          if (m_self.GetOpenedQty()) {
            record % m_open.lastTradePrice   // 8
                % m_self.GetOpenAvgPrice();  // 9
          } else {
            record % '-' % '-';  // 8, 9
          }
          record % m_self.GetCurrency()                 // 10
              % m_self.GetPlanedQty()                   // 11
              % m_self.GetOpenedQty()                   // 12
              % m_security.GetBidPriceValue()           // 13
              % m_security.GetAskPriceValue()           // 14
              % m_operationId                           // 15
              % order.transactionContext->GetOrderId()  // 16
              % tsOrderId;                              // 17 and last
        });
  }

  void ReportClosingStart(const char *eventDesc,
                          const boost::optional<OrderId> &id,
                          const Qty &maxQty) const noexcept {
    m_strategy.GetTradingLog().Write(
        "order\tclose->%1%\t%2%\t%3%\t%4%.%5%"
        "\tclose-type=%6%\tprice=%7$.8f->%8$.8f\t%9%"
        "\tmax-qty=%10$.8f\tactive-qty=%11$.8f\tbid/ask=%12$.8f/%13$.8f"
        "\tpos=%14%\torder=%15%/-",
        [&](TradingRecord &record) {
          record % eventDesc                                         // 1
              % m_self.GetCloseOrderSide()                           // 2
              % m_security.GetSymbol().GetSymbol().c_str()           // 3
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 4
              % m_tradingSystem.GetMode()                            // 5
              % m_closeReason                                        // 6
              % m_self.GetCloseStartPrice();                         // 7
          const auto &order = m_close.orders.back();
          if (order.price) {
            record % *order.price;  // 8
          } else {
            record % '-';  // 8
          }
          record % m_self.GetCurrency()        // 9
              % maxQty                         // 10
              % m_self.GetActiveQty()          // 11
              % m_security.GetBidPriceValue()  // 12
              % m_security.GetAskPriceValue()  // 13
              % m_operationId;                 // 14
          if (id) {
            record % *id;  // 15 and last
          } else {
            record % '-';  // 15 and last
          }
        });
  }

  void ReportClosingUpdate(const std::string &tsOrderId,
                           const OrderStatus &orderStatus) const noexcept {
    Assert(!m_close.orders.empty());
    m_strategy.GetTradingLog().Write(
        "order\tclose->%1%\t%2%\t%3%\t%4%.%5%\t%6%"
        "\tprice=%7$.8f->%8$.8f->%9$.8f(avg=%10$.8f)\t%11%"
        "\tqty=%12$.8f-%13$.8f=%14$.8f\tbid/ask=%15$.8f/%16$.8f"
        "\tpos=%17%\torder=%18%/%19%",
        [this, &tsOrderId, &orderStatus](TradingRecord &record) {
          const auto &order = m_close.orders.back();
          record % orderStatus                                       // 1
              % m_self.GetCloseOrderSide()                           // 2
              % m_security.GetSymbol().GetSymbol().c_str()           // 3
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 4
              % m_tradingSystem.GetMode()                            // 5
              % m_closeReason                                        // 6
              % m_self.GetCloseStartPrice();                         // 7
          if (order.price) {
            record % *order.price;  // 8
          } else {
            record % '-';  // 8
          }
          if (m_self.GetClosedQty()) {
            record % m_close.lastTradePrice   // 9
                % m_self.GetCloseAvgPrice();  // 10
          } else {
            record % '-' % '-';  // 9, 10
          }
          record % m_self.GetCurrency()                 // 11
              % m_self.GetOpenedQty()                   // 12
              % m_self.GetClosedQty()                   // 13
              % m_self.GetActiveQty()                   // 14
              % m_security.GetBidPriceValue()           // 15
              % m_security.GetAskPriceValue()           // 16
              % m_operationId                           // 17
              % order.transactionContext->GetOrderId()  // 18
              % tsOrderId;                              // 19 and last
        });
  }

 public:
  void RestoreOpenState(const Price &openPrice) {
    const auto now = m_strategy.GetContext().GetCurrentTime();

    if (m_self.IsCancelling()) {
      throw Exception(
          "Failed to restore open-stat as canceling is not completed");
    } else if (m_self.IsOpened() || !m_close.orders.empty() ||
               m_self.IsError()) {
      throw AlreadyStartedError();
    }

    Assert(!m_self.IsClosed());
    Assert(!m_self.IsCompleted());
    Assert(!m_self.HasActiveOrders());
    AssertLt(0, m_planedQty);
    AssertEq(0, m_open.qty);
    AssertLt(0, m_open.startPrice);
    Assert(m_open.time.is_not_a_date_time());
    AssertEq(0, m_open.volume);
    AssertEq(0, m_open.numberOfTrades);
    AssertEq(0, m_open.lastTradePrice);
    AssertEq(0, m_open.orders.size());
    AssertEq(0, m_close.qty);
    AssertEq(0, m_close.startPrice);
    Assert(m_close.time.is_not_a_date_time());
    AssertEq(0, m_close.volume);
    AssertEq(0, m_close.numberOfTrades);
    AssertEq(0, m_close.lastTradePrice);
    AssertEq(0, m_close.orders.size());

    if (!m_isRegistered) {
      m_strategy.Register(m_self);
      // supporting prev. logic (when was m_strategy = nullptr),
      // don't know why set flag in other place.
    }

    if (!m_security.GetSymbol().IsExplicit()) {
      m_expiration = m_security.GetExpiration();
    }

    m_open.orders.emplace_back(std::move(now), boost::none, m_planedQty);
    try {
      auto &order = m_open.orders.back();
      order.isActive = false;

      m_open.time = order.time;
      m_open.OnNewTrade(TradingSystem::TradeInfo{openPrice, order.qty},
                        openPrice);

      m_isRegistered = true;  // supporting prev. logic
                              // (when was m_strategy = nullptr),
                              // don't know why set flag only here.
    } catch (...) {
      if (m_isRegistered) {
        m_strategy.Unregister(m_self);
      }
      m_open.orders.pop_back();
      throw;
    }

    ReportOpeningStateRestpration();
  }

  template <typename OpenImpl>
  const TransactionContext &Open(const OpenImpl &openImpl,
                                 const OrderParams &orderParams,
                                 boost::optional<Price> &&price) {
    const auto now = m_strategy.GetContext().GetCurrentTime();

    if (!m_security.IsOnline()) {
      throw Exception("Security is not online");
    } else if (!m_security.IsTradingSessionOpened()) {
      throw Exception("Security trading session is closed");
    } else if (m_self.IsCancelling()) {
      throw Exception("Failed to start opening as canceling is not completed");
    } else if (!m_close.orders.empty()) {
      throw AlreadyStartedError();
    }

    Assert(!m_self.IsFullyOpened());
    Assert(!m_self.IsError());
    Assert(!m_self.IsClosed());

    AssertGt(m_planedQty, m_open.qty);
    auto qty = m_planedQty - m_open.qty;

    if (!m_isRegistered) {
      m_strategy.Register(m_self);
      // supporting prev. logic (when was m_strategy = nullptr),
      // don't know why set flag in other place.
    }

    m_open.orders.emplace_back(std::move(now), std::move(price), qty);
    ReportOpeningStart("start", boost::none);
    auto &order = m_open.orders.back();

    try {
      if (!orderParams.expiration && !m_security.GetSymbol().IsExplicit()) {
        m_expiration = m_security.GetExpiration();
        OrderParams additionalOrderParams(orderParams);
        additionalOrderParams.expiration = &*m_expiration;
        order.transactionContext = openImpl(qty, additionalOrderParams);
      } else {
        order.transactionContext = openImpl(qty, orderParams);
      }
      Assert(order.transactionContext);

      ReportOpeningStart("sent", order.transactionContext->GetOrderId());

      m_isRegistered = true;  // supporting prev. logic
                              // (when was m_strategy = nullptr),
                              // don't know why set flag only here.

    } catch (...) {
      if (m_isRegistered) {
        m_strategy.Unregister(m_self);
      }
      try {
      } catch (...) {
        AssertFailNoException();
      }
      m_open.orders.pop_back();
      throw;
    }

    return *order.transactionContext;
  }

  template <typename CloseImpl>
  const TransactionContext &Close(const CloseImpl &closeImpl,
                                  boost::optional<Price> &&price,
                                  const Qty &maxQty,
                                  const OrderParams &orderParams) {
    AssertNe(CLOSE_REASON_NONE, m_closeReason);

    const auto now = m_strategy.GetContext().GetCurrentTime();

    if (!m_self.IsOpened()) {
      throw NotOpenedError();
    } else if (m_self.HasActiveCloseOrders() || m_self.IsClosed()) {
      throw AlreadyClosedError();
    } else if (m_self.IsCancelling()) {
      throw Exception("Failed to start closing as canceling is not completed");
    }

    Assert(m_isRegistered);
    Assert(m_self.IsStarted());
    Assert(!m_self.IsError());
    Assert(!m_self.HasActiveOrders());
    Assert(!m_self.IsCompleted());

    if (!m_close.startPrice) {
      m_close.startPrice = m_self.GetMarketClosePrice();
      Assert(m_close.startPrice);
    }

    m_close.orders.emplace_back(std::move(now), std::move(price),
                                std::min<Qty>(maxQty, m_self.GetActiveQty()));
    Order &order = m_close.orders.back();

    ReportClosingStart("start", boost::none, maxQty);

    try {
      if (m_expiration && !orderParams.expiration) {
        OrderParams additionalOrderParams(orderParams);
        additionalOrderParams.expiration = &*m_expiration;
        order.transactionContext = closeImpl(order.qty, additionalOrderParams);
      } else {
        order.transactionContext = closeImpl(order.qty, orderParams);
      }
      Assert(order.transactionContext);

      ReportClosingStart("sent", order.transactionContext->GetOrderId(),
                         maxQty);

    } catch (...) {
      m_close.orders.pop_back();
      throw;
    }

    return *order.transactionContext;
  }

  template <typename Orders>
  bool CancelOrder(Orders &orders, const char *const direction) {
    if (!orders.HasActiveOrders() || m_open.IsCanceling()) {
      return false;
    }
    Assert(!orders.orders.empty());
    auto &order = orders.orders.back();
    Assert(order.isActive);
    Assert(!order.isCanceled);
    m_strategy.GetTradingLog().Write(
        "order\tcancel-all\t%1%-order\t%2%\t%3%\t%4%"
        "\tpos=%5%\torder=%6%",
        [this, &order, &direction](TradingRecord &record) {
          record % direction                                         // 1
              % m_security.GetSymbol().GetSymbol().c_str()           // 2
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 3
              % m_tradingSystem.GetMode()                            // 4
              % m_operationId                                        // 5
              % order.transactionContext->GetOrderId();              // 6
        });
    m_tradingSystem.CancelOrder(order.transactionContext->GetOrderId());
    order.isCanceled = true;
    return true;
  }
};

//////////////////////////////////////////////////////////////////////////

namespace {
class DummyPositionOperationContext : public PositionOperationContext {
 public:
  virtual ~DummyPositionOperationContext() override = default;

 public:
  virtual const OrderPolicy &GetOpenOrderPolicy() const override {
    throw LogicError("Position instance does not use operation context");
  }
  virtual const OrderPolicy &GetCloseOrderPolicy() const override {
    throw LogicError("Position instance does not use operation context");
  }
  virtual void Setup(Position &) const override {
    throw LogicError("Position instance does not use operation context");
  }
  virtual bool IsLong() const override {
    throw LogicError("Position instance does not use operation context");
  }
  virtual Qty GetPlannedQty() const override {
    throw LogicError("Position instance does not use operation context");
  }
  virtual bool HasCloseSignal(const Position &) const override {
    throw LogicError("Position instance does not use operation context");
  }
  virtual boost::shared_ptr<PositionOperationContext> StartInvertedPosition(
      const Position &) override {
    throw LogicError("Position instance does not use operation context");
  }
};
}
Position::Position(Strategy &strategy,
                   const uuids::uuid &operationId,
                   int64_t subOperationId,
                   TradingSystem &tradingSystem,
                   Security &security,
                   const Currency &currency,
                   const Qty &qty,
                   const Price &startPrice,
                   const TimeMeasurement::Milestones &timeMeasurement)
    : m_pimpl(std::make_unique<Implementation>(
          *this,
          boost::make_shared<DummyPositionOperationContext>(),
          tradingSystem,
          strategy,
          operationId,
          subOperationId,
          security,
          currency,
          qty,
          startPrice,
          timeMeasurement)) {
  Assert(!strategy.IsBlocked());
}

Position::Position(
    const boost::shared_ptr<PositionOperationContext> &operationContext,
    Strategy &strategy,
    const uuids::uuid &operationId,
    int64_t subOperationId,
    TradingSystem &tradingSystem,
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &startPrice,
    const TimeMeasurement::Milestones &timeMeasurement)
    : m_pimpl(std::make_unique<Implementation>(*this,
                                               operationContext,
                                               tradingSystem,
                                               strategy,
                                               operationId,
                                               subOperationId,
                                               security,
                                               currency,
                                               qty,
                                               startPrice,
                                               timeMeasurement)) {
  Assert(!strategy.IsBlocked());
}

Position::~Position() = default;

const uuids::uuid &Position::GetId() const { return m_pimpl->m_operationId; }

bool Position::IsLong() const {
  static_assert(numberOfTypes == 2, "List changed.");
  Assert(GetType() == Position::TYPE_LONG || GetType() == Position::TYPE_SHORT);
  return GetType() == Position::TYPE_LONG;
}

PositionOperationContext &Position::GetOperationContext() {
  return *m_pimpl->m_operationContext;
}
const trdk::PositionOperationContext &Position::GetOperationContext() const {
  return const_cast<Position *>(this)->GetOperationContext();
}

const ContractExpiration &Position::GetExpiration() const {
  if (!m_pimpl->m_expiration) {
    Assert(m_pimpl->m_expiration);
    boost::format error("Position %1% %2% does not have expiration");
    error % GetSecurity() % GetId();
    throw LogicError(error.str().c_str());
  }
  return *m_pimpl->m_expiration;
}

const Strategy &Position::GetStrategy() const noexcept {
  return const_cast<Position *>(this)->GetStrategy();
}

Strategy &Position::GetStrategy() noexcept { return m_pimpl->m_strategy; }

const Security &Position::GetSecurity() const noexcept {
  return const_cast<Position *>(this)->GetSecurity();
}
Security &Position::GetSecurity() noexcept { return m_pimpl->m_security; }

const TradingSystem &Position::GetTradingSystem() const {
  return const_cast<Position *>(this)->GetTradingSystem();
}

TradingSystem &Position::GetTradingSystem() { return m_pimpl->m_tradingSystem; }

const Currency &Position::GetCurrency() const { return m_pimpl->m_currency; }

const TimeMeasurement::Milestones &Position::GetTimeMeasurement() {
  return m_pimpl->m_timeMeasurement;
}

const CloseReason &Position::GetCloseReason() const noexcept {
  return m_pimpl->m_closeReason;
}

void Position::SetCloseReason(const CloseReason &newCloseReason) {
  AssertNe(CLOSE_REASON_NONE, newCloseReason);
  if (m_pimpl->m_closeReason != CLOSE_REASON_NONE) {
    return;
  }
  GetOperationContext().OnCloseReasonChange(m_pimpl->m_closeReason,
                                            newCloseReason);
  m_pimpl->m_closeReason = newCloseReason;
}

void Position::ResetCloseReason(const CloseReason &newReason) {
  GetOperationContext().OnCloseReasonChange(m_pimpl->m_closeReason, newReason);
  m_pimpl->m_closeReason = newReason;
}

bool Position::IsFullyOpened() const {
  return GetActiveQty() >= GetPlanedQty();
}

bool Position::IsOpened() const noexcept {
  return !HasActiveOpenOrders() && GetOpenedQty() > 0;
}
bool Position::IsClosed() const noexcept {
  return !HasActiveOrders() && GetOpenedQty() > 0 && GetActiveQty() == 0;
}

bool Position::IsStarted() const noexcept {
  return !m_pimpl->m_open.orders.empty();
}

bool Position::IsCompleted() const noexcept {
  return m_pimpl->m_isMarketAsCompleted ||
         (IsStarted() && !HasActiveOrders() && GetActiveQty() == 0);
}

void Position::MarkAsCompleted() {
  Assert(!m_pimpl->m_isMarketAsCompleted);
  m_pimpl->m_isMarketAsCompleted = true;
  m_pimpl->m_strategy.OnPositionMarkedAsCompleted(*this);
}

bool Position::IsError() const noexcept { return m_pimpl->m_isError; }

bool Position::IsInactive() const noexcept { return m_pimpl->m_isInactive; }

void Position::ResetInactive() {
  Assert(m_pimpl->m_isInactive);
  m_pimpl->m_isInactive = false;
}

bool Position::IsCancelling() const {
  return m_pimpl->m_open.IsCanceling() || m_pimpl->m_close.IsCanceling();
}

bool Position::HasActiveOrders() const noexcept {
  return HasActiveCloseOrders() || HasActiveOpenOrders();
}
bool Position::HasActiveOpenOrders() const noexcept {
  return m_pimpl->m_open.HasActiveOrders();
}
bool Position::HasActiveCloseOrders() const noexcept {
  return m_pimpl->m_close.HasActiveOrders();
}

void Position::UpdateOpening(const OrderId &orderId,
                             const std::string &tradingSystemOrderId,
                             const OrderStatus &orderStatus,
                             const Qty &remainingQty,
                             const boost::optional<Volume> &commission,
                             const TradingSystem::TradeInfo *trade) {
  m_pimpl->UpdateOpening(orderId, tradingSystemOrderId, orderStatus,
                         remainingQty, commission, trade);
}

void Position::UpdateClosing(const OrderId &orderId,
                             const std::string &tradingSystemOrderId,
                             const OrderStatus &orderStatus,
                             const Qty &remainingQty,
                             const boost::optional<Volume> &commission,
                             const TradingSystem::TradeInfo *trade) {
  m_pimpl->UpdateClosing(orderId, tradingSystemOrderId, orderStatus,
                         remainingQty, commission, trade);
}

const pt::ptime &Position::GetOpenTime() const { return m_pimpl->m_open.time; }

Qty Position::GetActiveQty() const noexcept {
  AssertGe(GetOpenedQty(), GetClosedQty());
  return GetOpenedQty() - GetClosedQty();
}

Volume Position::GetActiveVolume() const {
  const auto &activeQty = GetActiveQty();
  if (activeQty == GetOpenedQty()) {
    // actual:
    return GetOpenedVolume();
  }
  if (!m_pimpl->m_open.qty) {
    return 0;
  }
  // approximate:
  return RoundByPrecision(activeQty * (GetOpenedVolume() / m_pimpl->m_open.qty),
                          GetSecurity().GetPricePrecisionPower());
}

const Qty &Position::GetClosedQty() const noexcept {
  return m_pimpl->m_close.qty;
}

Volume Position::GetClosedVolume() const { return m_pimpl->m_close.volume; }

const pt::ptime &Position::GetCloseTime() const {
  return m_pimpl->m_close.time;
}

const boost::optional<Price> &Position::GetActiveOrderPrice() const {
  Assert(!m_pimpl->m_open.HasActiveOrders() ||
         !m_pimpl->m_close.HasActiveOrders());
  if (m_pimpl->m_close.HasActiveOrders()) {
    Assert(!m_pimpl->m_open.HasActiveOrders());
    return m_pimpl->m_close.orders.back().price;
  } else if (m_pimpl->m_open.HasActiveOrders()) {
    return m_pimpl->m_open.orders.back().price;
  } else {
    throw Exception("Position has no active order");
  }
}

const pt::ptime &Position::GetActiveOrderTime() const {
  Assert(!m_pimpl->m_open.HasActiveOrders() ||
         !m_pimpl->m_close.HasActiveOrders());
  if (m_pimpl->m_close.HasActiveOrders()) {
    Assert(!m_pimpl->m_open.HasActiveOrders());
    return m_pimpl->m_close.orders.back().time;
  } else if (m_pimpl->m_open.HasActiveOrders()) {
    return m_pimpl->m_open.orders.back().time;
  } else {
    throw Exception("Position has no active order");
  }
}

const Price &Position::GetLastTradePrice() const {
  if (m_pimpl->m_close.numberOfTrades) {
    return m_pimpl->m_close.lastTradePrice;
  } else if (m_pimpl->m_open.numberOfTrades) {
    return m_pimpl->m_open.lastTradePrice;
  } else {
    throw Exception("Position has no trades");
  }
}

Volume Position::GetRealizedPnlVolume() const {
  return RoundByPrecision(
      GetRealizedPnl() * GetSecurity().GetNumberOfItemsPerQty(),
      GetSecurity().GetPricePrecisionPower());
}

Double Position::GetRealizedPnlPercentage() const {
  const auto ratio = GetRealizedPnlRatio();
  const auto result = ratio >= 1 ? ratio - 1 : -(1 - ratio);
  return result * 100;
}

Volume Position::GetPlannedPnl() const {
  return RoundByPrecision(GetUnrealizedPnl() + GetRealizedPnl(),
                          GetSecurity().GetPricePrecisionPower());
}

bool Position::IsProfit() const {
  const auto ratio = GetRealizedPnlRatio();
  return ratio > 1.0 && !IsEqual(ratio, 1.0);
}

Volume Position::CalcCommission() const {
  Volume result = 0;
  for (const auto &order : m_pimpl->m_open.orders) {
    result += order.commission;
  }
  for (const auto &order : m_pimpl->m_close.orders) {
    result += order.commission;
  }
  return result;
}

size_t Position::GetNumberOfOpenOrders() const {
  return m_pimpl->m_open.orders.size();
}
size_t Position::GetNumberOfOpenTrades() const {
  return m_pimpl->m_open.numberOfTrades;
}

size_t Position::GetNumberOfCloseOrders() const {
  return m_pimpl->m_close.orders.size();
}
size_t Position::GetNumberOfCloseTrades() const {
  return m_pimpl->m_close.numberOfTrades;
}

Position::StateUpdateConnection Position::Subscribe(
    const StateUpdateSlot &slot) const {
  return StateUpdateConnection(m_pimpl->m_stateUpdateSignal.connect(slot));
}

void Position::AttachAlgo(std::unique_ptr<Algo> &&algo) {
  Assert(algo);
  m_pimpl->m_algos.emplace_back(std::move(algo));
  m_pimpl->m_algos.back()->Report(*this, GetStrategy().GetTradingLog());
}

void Position::RunAlgos() {
  if (IsCompleted()) {
    return;
  }
  for (const auto &algo : m_pimpl->m_algos) {
    if (IsCancelling()) {
      break;
    }
    Assert(!IsCompleted());
    algo->Run();
  }
}

const Qty &Position::GetPlanedQty() const { return m_pimpl->m_planedQty; }

const Price &Position::GetOpenStartPrice() const {
  return m_pimpl->m_open.startPrice;
}

const pt::ptime &Position::GetOpenStartTime() const {
  if (m_pimpl->m_open.orders.empty()) {
    throw Exception("Position has no open-order");
  }
  return m_pimpl->m_open.orders.front().time;
}

const Qty &Position::GetOpenedQty() const noexcept {
  return m_pimpl->m_open.qty;
}
void Position::SetOpenedQty(const Qty &newQty) const noexcept {
  m_pimpl->m_open.qty = newQty;
  if (newQty > m_pimpl->m_planedQty) {
    m_pimpl->m_planedQty = newQty;
  }
}

Price Position::GetOpenAvgPrice() const {
  if (!m_pimpl->m_open.qty) {
    throw Exception("Position has no open price");
  }
  return m_pimpl->m_open.volume / m_pimpl->m_open.qty;
}

const boost::optional<Price> &Position::GetActiveOpenOrderPrice() const {
  Assert(!m_pimpl->m_open.HasActiveOrders() ||
         !m_pimpl->m_close.HasActiveOrders());
  if (!m_pimpl->m_open.HasActiveOrders()) {
    throw Exception("Position has no active open-order");
  }
  Assert(!m_pimpl->m_close.HasActiveOrders());
  return m_pimpl->m_open.orders.back().price;
}

const pt::ptime &Position::GetActiveOpenOrderTime() const {
  Assert(!m_pimpl->m_open.HasActiveOrders() ||
         !m_pimpl->m_close.HasActiveOrders());
  if (!m_pimpl->m_open.HasActiveOrders()) {
    throw Exception("Position has no active open-order");
  }
  Assert(!m_pimpl->m_close.HasActiveOrders());
  return m_pimpl->m_open.orders.back().time;
}

const Price &Position::GetLastOpenTradePrice() const {
  if (!m_pimpl->m_open.numberOfTrades) {
    throw Exception("Position has no open trades");
  }
  return m_pimpl->m_open.lastTradePrice;
}

Volume Position::GetOpenedVolume() const { return m_pimpl->m_open.volume; }

const Price &Position::GetCloseStartPrice() const {
  return m_pimpl->m_close.startPrice;
}

const pt::ptime &Position::GetCloseStartTime() const {
  if (m_pimpl->m_close.orders.empty()) {
    throw Exception("Position has no close-order");
  }
  return m_pimpl->m_close.orders.front().time;
}

void Position::SetCloseStartPrice(const Price &price) {
  m_pimpl->m_close.startPrice = price;
}

Price Position::GetCloseAvgPrice() const {
  if (!m_pimpl->m_close.qty) {
    throw Exception("Position has no close price");
  }
  return m_pimpl->m_close.volume / m_pimpl->m_close.qty;
}

const boost::optional<trdk::Price> &Position::GetActiveCloseOrderPrice() const {
  Assert(!m_pimpl->m_open.HasActiveOrders() ||
         !m_pimpl->m_close.HasActiveOrders());
  if (!m_pimpl->m_close.HasActiveOrders()) {
    throw Exception("Position has no active close-order");
  }
  Assert(!m_pimpl->m_open.HasActiveOrders());
  return m_pimpl->m_close.orders.back().price;
}

const pt::ptime &Position::GetActiveCloseOrderTime() const {
  Assert(!m_pimpl->m_open.HasActiveOrders() ||
         !m_pimpl->m_close.HasActiveOrders());
  if (!m_pimpl->m_close.HasActiveOrders()) {
    throw Exception("Position has no active close-order");
  }
  Assert(!m_pimpl->m_open.HasActiveOrders());
  return m_pimpl->m_close.orders.back().time;
}

const Price &Position::GetLastCloseTradePrice() const {
  if (!m_pimpl->m_close.numberOfTrades) {
    throw Exception("Position has no close trades");
  }
  return m_pimpl->m_close.lastTradePrice;
}

void Position::RestoreOpenState(const trdk::Price &openPrice) {
  m_pimpl->RestoreOpenState(openPrice);
}

const TransactionContext &Position::OpenAtMarketPrice() {
  return OpenAtMarketPrice(m_pimpl->m_defaultOrderParams);
}

const TransactionContext &Position::OpenAtMarketPrice(
    const OrderParams &params) {
  return m_pimpl->Open(
      [this](const Qty &qty, const OrderParams &params) {
        return DoOpenAtMarketPrice(qty, params);
      },
      params, boost::none);
}

const TransactionContext &Position::Open(const Price &price) {
  return Open(price, m_pimpl->m_defaultOrderParams);
}

const TransactionContext &Position::Open(const Price &price,
                                         const OrderParams &params) {
  return m_pimpl->Open(
      [this, &price](const Qty &qty, const OrderParams &params) {
        return DoOpen(qty, price, params);
      },
      params, price);
}

const TransactionContext &Position::OpenImmediatelyOrCancel(
    const Price &price) {
  return OpenImmediatelyOrCancel(price, m_pimpl->m_defaultOrderParams);
}

const TransactionContext &Position::OpenImmediatelyOrCancel(
    const Price &price, const OrderParams &params) {
  return m_pimpl->Open(
      [this, price](const Qty &qty, const OrderParams &params) {
        return DoOpenImmediatelyOrCancel(qty, price, params);
      },
      params, price);
}

const TransactionContext &Position::CloseAtMarketPrice() {
  return CloseAtMarketPrice(m_pimpl->m_defaultOrderParams);
}

const TransactionContext &Position::CloseAtMarketPrice(
    const OrderParams &params) {
  return m_pimpl->Close(
      [this](const Qty &qty, const OrderParams &params) {
        return DoCloseAtMarketPrice(qty, params);
      },
      boost::none, GetActiveQty(), params);
}

const TransactionContext &Position::Close(const Price &price) {
  return Close(price, GetActiveQty(), m_pimpl->m_defaultOrderParams);
}

const TransactionContext &Position::Close(const Price &price,
                                          const Qty &maxQty) {
  return Close(price, maxQty, m_pimpl->m_defaultOrderParams);
}

const TransactionContext &Position::Close(const Price &price,
                                          const OrderParams &params) {
  return Close(price, GetActiveQty(), params);
}

const TransactionContext &Position::Close(const Price &price,
                                          const Qty &maxQty,
                                          const OrderParams &params) {
  return m_pimpl->Close(
      [this, &price](const Qty &qty, const OrderParams &params) {
        return DoClose(qty, price, params);
      },
      price, maxQty, params);
}

const TransactionContext &Position::CloseImmediatelyOrCancel(
    const Price &price) {
  return CloseImmediatelyOrCancel(price, m_pimpl->m_defaultOrderParams);
}

const TransactionContext &Position::CloseImmediatelyOrCancel(
    const Price &price, const OrderParams &params) {
  return m_pimpl->Close(
      [this, price](const Qty &qty, const OrderParams &params) {
        return DoCloseImmediatelyOrCancel(qty, price, params);
      },
      price, GetActiveQty(), params);
}

bool Position::CancelAllOrders() {
  bool isCancelRequestSent = m_pimpl->CancelOrder(m_pimpl->m_open, "open");
  if (m_pimpl->CancelOrder(m_pimpl->m_close, "close")) {
    isCancelRequestSent = true;
  }
  return isCancelRequestSent;
}

const char *trdk::ConvertToPch(const Position::Type &type) {
  static_assert(Position::numberOfTypes == 2, "Close type list changed.");
  switch (type) {
    default:
      AssertEq(Position::TYPE_LONG, type);
      return "unknown";
    case Position::TYPE_LONG:
      return "long";
    case Position::TYPE_SHORT:
      return "short";
  }
}

//////////////////////////////////////////////////////////////////////////

LongPosition::LongPosition(Strategy &strategy,
                           const uuids::uuid &operationId,
                           int64_t subOperationId,
                           TradingSystem &tradingSystem,
                           Security &security,
                           const Currency &currency,
                           const Qty &qty,
                           const Price &startPrice,
                           const TimeMeasurement::Milestones &timeMeasurement)
    : Position(strategy,
               operationId,
               subOperationId,
               tradingSystem,
               security,
               currency,
               qty,
               startPrice,
               timeMeasurement) {
  GetStrategy().GetTradingLog().Write(
      "position\tnew\tlong\t%1%\t%2%.%3%\tprice=%4$.8f\t%5%\tqty=%6$.8f"
      "\tpos=%7%",
      [this](TradingRecord &record) {
        record % GetSecurity().GetSymbol().GetSymbol().c_str()  // 1
            % GetTradingSystem().GetInstanceName().c_str()      // 2
            % GetTradingSystem().GetMode()                      // 3
            % GetOpenStartPrice()                               // 4
            % GetCurrency()                                     // 5
            % GetPlanedQty()                                    // 6
            % GetId();                                          // 7 and last
      });
}

LongPosition::LongPosition(
    const boost::shared_ptr<PositionOperationContext> &operationContext,
    Strategy &strategy,
    const uuids::uuid &operationId,
    int64_t subOperationId,
    TradingSystem &tradingSystem,
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &startPrice,
    const TimeMeasurement::Milestones &timeMeasurement)
    : Position(operationContext,
               strategy,
               operationId,
               subOperationId,
               tradingSystem,
               security,
               currency,
               qty,
               startPrice,
               timeMeasurement) {
  GetStrategy().GetTradingLog().Write(
      "position\tnew\tlong\t%1%\t%2%.%3%\tprice=%4$.8f\t%5%\tqty=%6$.8f"
      "\tpos=%7%",
      [this](TradingRecord &record) {
        record % GetSecurity().GetSymbol().GetSymbol().c_str()  // 1
            % GetTradingSystem().GetInstanceName().c_str()      // 2
            % GetTradingSystem().GetMode()                      // 3
            % GetOpenStartPrice()                               // 4
            % GetCurrency()                                     // 5
            % GetPlanedQty()                                    // 6
            % GetId();                                          // 7 and last
      });
}

LongPosition::~LongPosition() {
  GetStrategy().GetTradingLog().Write(
      "position\tdel\tlong\tpos=%1%",
      [this](TradingRecord &record) { record % GetId(); });
}

LongPosition::Type LongPosition::GetType() const { return TYPE_LONG; }

OrderSide LongPosition::GetOpenOrderSide() const { return ORDER_SIDE_BUY; }

OrderSide LongPosition::GetCloseOrderSide() const { return ORDER_SIDE_SELL; }

Volume LongPosition::GetRealizedPnl() const {
  const auto &activeQty = GetActiveQty();
  if (activeQty == 0) {
    return RoundByPrecision(GetClosedVolume() - GetOpenedVolume(),
                            GetSecurity().GetPricePrecisionPower());
  }
  const auto openedVolume = (GetOpenedQty() - activeQty) * GetOpenAvgPrice();
  return RoundByPrecision(GetClosedVolume() - openedVolume,
                          GetSecurity().GetPricePrecisionPower());
}

Double LongPosition::GetRealizedPnlRatio() const {
  const auto &activeQty = GetActiveQty();
  if (activeQty == 0) {
    const auto openedVolume = GetOpenedVolume();
    if (openedVolume == 0) {
      return 0;
    }
    return GetClosedVolume() / openedVolume;
  }
  const auto openPrice = GetOpenAvgPrice();
  if (!openPrice) {
    return 0;
  }
  const auto openedVolume = (GetOpenedQty() - activeQty) * openPrice;
  return GetClosedVolume() / openedVolume;
}

Volume LongPosition::GetUnrealizedPnl() const {
  return RoundByPrecision(
      (GetActiveQty() * GetSecurity().GetBidPrice()) - GetActiveVolume(),
      GetSecurity().GetPricePrecisionPower());
}

Price LongPosition::GetMarketOpenPrice() const {
  return GetSecurity().GetAskPrice();
}

Price LongPosition::GetMarketClosePrice() const {
  return GetSecurity().GetBidPrice();
}

Price LongPosition::GetMarketOpenOppositePrice() const {
  return GetSecurity().GetBidPrice();
}

Price LongPosition::GetMarketCloseOppositePrice() const {
  return GetSecurity().GetAskPrice();
}

boost::shared_ptr<const TransactionContext> LongPosition::DoOpenAtMarketPrice(
    const Qty &qty, const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, boost::none, params,
      boost::bind(&LongPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY, TIME_IN_FORCE_GTC,
      GetTimeMeasurement());
}

boost::shared_ptr<const TransactionContext> LongPosition::DoOpen(
    const Qty &qty, const Price &price, const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&LongPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY, TIME_IN_FORCE_GTC,
      GetTimeMeasurement());
}

boost::shared_ptr<const TransactionContext>
LongPosition::DoOpenImmediatelyOrCancel(const Qty &qty,
                                        const Price &price,
                                        const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&LongPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY, TIME_IN_FORCE_IOC,
      GetTimeMeasurement());
}

boost::shared_ptr<const TransactionContext> LongPosition::DoCloseAtMarketPrice(
    const Qty &qty, const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, boost::none, params,
      boost::bind(&LongPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL, TIME_IN_FORCE_GTC,
      GetTimeMeasurement());
}

boost::shared_ptr<const TransactionContext> LongPosition::DoClose(
    const Qty &qty, const Price &price, const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&LongPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL, TIME_IN_FORCE_GTC,

      GetTimeMeasurement());
}

boost::shared_ptr<const TransactionContext>
LongPosition::DoCloseImmediatelyOrCancel(const Qty &qty,
                                         const Price &price,
                                         const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  AssertLt(0, qty);
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&LongPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL, TIME_IN_FORCE_IOC,
      GetTimeMeasurement());
}

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(Strategy &strategy,
                             const uuids::uuid &operationId,
                             int64_t subOperationId,
                             TradingSystem &tradingSystem,
                             Security &security,
                             const Currency &currency,
                             const Qty &qty,
                             const Price &startPrice,
                             const TimeMeasurement::Milestones &timeMeasurement)
    : Position(strategy,
               operationId,
               subOperationId,
               tradingSystem,
               security,
               currency,
               qty,
               startPrice,
               timeMeasurement) {
  GetStrategy().GetTradingLog().Write(
      "position\tnew\tshort\t%1%\t%2%.%3%\tprice=%4$.8f\t%5%\tqty=%6$.8f"
      "\tpos=%7%",
      [this](TradingRecord &record) {
        record % GetSecurity().GetSymbol().GetSymbol().c_str()  // 1
            % GetTradingSystem().GetInstanceName().c_str()      // 2
            % GetTradingSystem().GetMode()                      // 3
            % GetOpenStartPrice()                               // 4
            % GetCurrency()                                     // 5
            % GetPlanedQty()                                    // 6
            % GetId();                                          // 7 and last
      });
}

ShortPosition::ShortPosition(
    const boost::shared_ptr<PositionOperationContext> &operationContext,
    Strategy &strategy,
    const uuids::uuid &operationId,
    int64_t subOperationId,
    TradingSystem &tradingSystem,
    Security &security,
    const Currency &currency,
    const Qty &qty,
    const Price &startPrice,
    const TimeMeasurement::Milestones &timeMeasurement)
    : Position(operationContext,
               strategy,
               operationId,
               subOperationId,
               tradingSystem,
               security,
               currency,
               qty,
               startPrice,
               timeMeasurement) {
  GetStrategy().GetTradingLog().Write(
      "position\tnew\tshort\t%1%\t%2%.%3%\tprice=%4$.8f\t%5%\tqty=%6$.8f"
      "\tpos=%7%",
      [this](TradingRecord &record) {
        record % GetSecurity().GetSymbol().GetSymbol().c_str()  // 1
            % GetTradingSystem().GetInstanceName().c_str()      // 2
            % GetTradingSystem().GetMode()                      // 3
            % GetOpenStartPrice()                               // 4
            % GetCurrency()                                     // 5
            % GetPlanedQty()                                    // 6
            % GetId();                                          // 7 and last
      });
}

ShortPosition::~ShortPosition() {
  GetStrategy().GetTradingLog().Write(
      "position\tdel\tshort\tpos=%1%",
      [this](TradingRecord &record) { record % GetId(); });
}

ShortPosition::Type ShortPosition::GetType() const { return TYPE_SHORT; }

OrderSide ShortPosition::GetOpenOrderSide() const { return ORDER_SIDE_SELL; }

OrderSide ShortPosition::GetCloseOrderSide() const { return ORDER_SIDE_BUY; }

Volume ShortPosition::GetRealizedPnl() const {
  if (GetActiveQty() == 0) {
    return RoundByPrecision(GetOpenedVolume() - GetClosedVolume(),
                            GetSecurity().GetPricePrecisionPower());
  }
  const auto openedVolume =
      (GetOpenedQty() - GetActiveQty()) * GetOpenAvgPrice();
  return RoundByPrecision(openedVolume - GetClosedVolume(),
                          GetSecurity().GetPricePrecisionPower());
}
Double ShortPosition::GetRealizedPnlRatio() const {
  const auto closedVolume = GetClosedVolume();
  if (closedVolume == 0) {
    return 0;
  }
  if (GetActiveQty() == 0) {
    return GetOpenedVolume() / closedVolume;
  }
  const auto openedVolume =
      (GetOpenedQty() - GetActiveQty()) * GetOpenAvgPrice();
  return openedVolume / closedVolume;
}
Volume ShortPosition::GetUnrealizedPnl() const {
  return RoundByPrecision(
      GetActiveVolume() - (GetActiveQty() * GetSecurity().GetAskPrice()),
      GetSecurity().GetPricePrecisionPower());
}

Price ShortPosition::GetMarketOpenPrice() const {
  return GetSecurity().GetBidPrice();
}

Price ShortPosition::GetMarketClosePrice() const {
  return GetSecurity().GetAskPrice();
}

Price ShortPosition::GetMarketOpenOppositePrice() const {
  return GetSecurity().GetAskPrice();
}

Price ShortPosition::GetMarketCloseOppositePrice() const {
  return GetSecurity().GetBidPrice();
}

boost::shared_ptr<const TransactionContext> ShortPosition::DoOpenAtMarketPrice(
    const Qty &qty, const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, boost::none, params,
      boost::bind(&ShortPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL, TIME_IN_FORCE_GTC,
      GetTimeMeasurement());
}

boost::shared_ptr<const TransactionContext> ShortPosition::DoOpen(
    const Qty &qty, const Price &price, const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&ShortPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL, TIME_IN_FORCE_GTC,
      GetTimeMeasurement());
}

boost::shared_ptr<const TransactionContext>
ShortPosition::DoOpenImmediatelyOrCancel(const Qty &qty,
                                         const Price &price,
                                         const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&ShortPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL, TIME_IN_FORCE_IOC,
      GetTimeMeasurement());
}

boost::shared_ptr<const TransactionContext> ShortPosition::DoCloseAtMarketPrice(
    const Qty &qty, const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, boost::none, params,
      boost::bind(&ShortPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY, TIME_IN_FORCE_GTC,
      GetTimeMeasurement());
}

boost::shared_ptr<const TransactionContext> ShortPosition::DoClose(
    const Qty &qty, const Price &price, const OrderParams &params) {
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&ShortPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY, TIME_IN_FORCE_GTC,
      GetTimeMeasurement());
}

boost::shared_ptr<const TransactionContext>
ShortPosition::DoCloseImmediatelyOrCancel(const Qty &qty,
                                          const Price &price,
                                          const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&ShortPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5, _6),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY, TIME_IN_FORCE_IOC,
      GetTimeMeasurement());
}

//////////////////////////////////////////////////////////////////////////
