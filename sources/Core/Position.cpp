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
#include "DropCopy.hpp"
#include "Settings.hpp"
#include "Strategy.hpp"
#include "TradingLog.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace pt = boost::posix_time;
namespace uuids = boost::uuids;
namespace sig = boost::signals2;

//////////////////////////////////////////////////////////////////////////

namespace {

const OrderParams defaultOrderParams;

class UuidGenerator : private boost::noncopyable {
 public:
  uuids::uuid operator()() { return m_generator(); }

 private:
  uuids::random_generator m_generator;
} generateUuid;
}

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

    const boost::optional<ScaledPrice> price;
    const Qty qty;

    uuids::uuid uuid;

    OrderId id;

    Qty executedQty;

    explicit Order(const pt::ptime &&time,
                   boost::optional<ScaledPrice> &&price,
                   const Qty &qty,
                   const uuids::uuid &&uuid)
        : time(std::move(time)),
          isActive(true),
          isCanceled(false),
          price(std::move(price)),
          qty(qty),
          uuid(std::move(uuid)),
          id(0),
          executedQty(0) {}
  };

  struct CloseOrder : public Order {
    explicit CloseOrder(const pt::ptime &&time,
                        boost::optional<ScaledPrice> &&price,
                        const Qty &qty,
                        const uuids::uuid &&uuid)
        : Order(std::move(time), std::move(price), qty, std::move(uuid)) {}
  };

  template <typename Order>
  struct DirectionData {
    ScaledPrice startPrice;

    pt::ptime time;

    ScaledPrice volume;
    Qty qty;
    size_t numberOfTrades;
    ScaledPrice lastTradePrice;
    std::vector<Order> orders;

    explicit DirectionData(const ScaledPrice &startPrice)
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

    void OnNewTrade(const TradingSystem::TradeInfo &trade) {
      AssertLt(0, trade.price);
      AssertLt(0, trade.qty);
      Assert(!orders.empty());
      auto &order = orders.back();
      order.executedQty += trade.qty;
      volume += ScaledPrice(trade.price * trade.qty);
      qty += trade.qty;
      ++numberOfTrades;
      lastTradePrice = trade.price;
    }
  };

  typedef DirectionData<Order> OpenData;
  typedef DirectionData<CloseOrder> CloseData;

 public:
  Position &m_self;

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

  explicit Implementation(Position &position,
                          TradingSystem &tradingSystem,
                          Strategy &strategy,
                          const uuids::uuid &operationId,
                          int64_t subOperationId,
                          Security &security,
                          const Currency &currency,
                          const Qty &qty,
                          const ScaledPrice &startPrice,
                          const TimeMeasurement::Milestones &timeMeasurement)
      : m_self(position),
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
                     const TradingSystem::TradeInfo *trade) {
    auto lock = m_strategy.LockForOtherThreads();

    Assert(!m_self.IsOpened());
    Assert(!m_self.IsClosed());
    Assert(!m_self.IsCompleted());
    Assert(!m_open.orders.empty());
    Assert(m_close.orders.empty());
    Assert(m_close.time.is_not_a_date_time());

    Order &order = m_open.orders.back();
    AssertGe(order.id, orderId);
    AssertGe(order.qty, remainingQty);
    AssertLe(order.qty, m_planedQty);
    Assert(order.isActive);
    if (order.id != orderId) {
      throw Exception("Unknown open order ID");
    }

    static_assert(numberOfOrderStatuses == 9, "List changed.");
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
        AssertEq(0, m_open.volume);
        Assert(!trade);
        AssertLt(0, remainingQty);
        ReportOpeningUpdate(tradingSystemOrderId, orderStatus);
        CopyOrder(&tradingSystemOrderId, true, orderStatus);
        return;
      case ORDER_STATUS_FILLED:
      case ORDER_STATUS_FILLED_PARTIALLY:
        Assert(trade);
        if (!trade) {
          throw Exception("Filled order has no trade information");
        }
        AssertEq(order.executedQty + trade->qty + remainingQty, order.qty);
        m_open.OnNewTrade(*trade);
        ReportOpeningUpdate(tradingSystemOrderId, orderStatus);
        CopyTrade(trade->id, m_security.DescalePrice(trade->price), trade->qty,
                  true);
        order.isActive = remainingQty > 0;
        break;
      case ORDER_STATUS_INACTIVE:
        order.isActive = false;
        ReportOpeningUpdate(tradingSystemOrderId, orderStatus);
        m_isInactive = true;
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

    CopyOrder(&tradingSystemOrderId, true, orderStatus);

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
    AssertEq(order.id, orderId);
    Assert(order.isActive);
    if (order.id != orderId) {
      throw Exception("Unknown open order ID");
    }

    static_assert(numberOfOrderStatuses == 9, "List changed.");
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
        ReportClosingUpdate(tradingSystemOrderId, orderStatus);
        return;
      case ORDER_STATUS_FILLED:
      case ORDER_STATUS_FILLED_PARTIALLY:
        Assert(trade);
        if (!trade) {
          throw Exception("Filled order has no trade information");
        }
        AssertEq(order.executedQty + trade->qty + remainingQty, order.qty);
        m_close.OnNewTrade(*trade);
        ReportClosingUpdate(tradingSystemOrderId, orderStatus);
        CopyTrade(trade->id, m_security.DescalePrice(trade->price), trade->qty,
                  false);
        if (remainingQty != 0) {
          CopyOrder(&tradingSystemOrderId, false, orderStatus);
          return;
        }
        break;
      case ORDER_STATUS_INACTIVE:
        ReportClosingUpdate(tradingSystemOrderId, orderStatus);
        m_isInactive = true;
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

    CopyOrder(&tradingSystemOrderId, false, orderStatus);

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
              % m_security.DescalePrice(m_open.startPrice);          // 5
          if (order.price) {
            record % m_security.DescalePrice(*order.price);  // 6
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
        "\tpos=%12%\torder=%13%/%14%/-",
        [this, eventDesc, &id](TradingRecord &record) {
          const auto &order = m_open.orders.back();
          record % eventDesc                                         // 1
              % m_self.GetOpenOrderSide()                            // 2
              % m_security.GetSymbol().GetSymbol().c_str()           // 3
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 4
              % m_tradingSystem.GetMode()                            // 5
              % m_security.DescalePrice(m_open.startPrice);          // 6
          if (order.price) {
            record % m_security.DescalePrice(*order.price);  // 7
          } else {
            record % '-';  // 7
          }
          record % m_self.GetCurrency()        // 8
              % m_self.GetPlanedQty()          // 9
              % m_security.GetBidPriceValue()  // 10
              % m_security.GetAskPriceValue()  // 11
              % m_operationId                  // 12
              % order.uuid;                    // 13
          if (id) {
            record % *id;  // 14 and last
          } else {
            record % '-';  // 14 and last
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
        "\tpos=%15%\torder=%16%/%17%/%18%",
        [this, &tsOrderId, &orderStatus](TradingRecord &record) {
          const auto &order = m_open.orders.back();
          record % orderStatus                                        // 1
              % m_self.GetOpenOrderSide()                             // 2
              % m_security.GetSymbol().GetSymbol().c_str()            // 3
              % m_self.GetTradingSystem().GetInstanceName().c_str()   // 4
              % m_tradingSystem.GetMode()                             // 5
              % m_security.DescalePrice(m_self.GetOpenStartPrice());  // 6
          if (order.price) {
            record % m_security.DescalePrice(*order.price);  // 7
          } else {
            record % '-';  // 7
          }
          if (m_self.GetOpenedQty()) {
            record % m_security.DescalePrice(m_open.lastTradePrice)   // 8
                % m_security.DescalePrice(m_self.GetOpenAvgPrice());  // 9
          } else {
            record % '-' % '-';  // 8, 9
          }
          record % m_self.GetCurrency()        // 10
              % m_self.GetPlanedQty()          // 11
              % m_self.GetOpenedQty()          // 12
              % m_security.GetBidPriceValue()  // 13
              % m_security.GetAskPriceValue()  // 14
              % m_operationId                  // 15
              % order.uuid                     // 16
              % order.id                       // 17
              % tsOrderId;                     // 18 and last
        });
  }

  void ReportClosingStart(const char *eventDesc,
                          const uuids::uuid &uuid,
                          const boost::optional<OrderId> &id,
                          const Qty &maxQty) const noexcept {
    m_strategy.GetTradingLog().Write(
        "order\tclose->%1%\t%2%\t%3%\t%4%.%5%"
        "\tclose-type=%6%\tprice=%7$.8f->%8$.8f\t%9%"
        "\tmax-qty=%10$.8f\tactive-qty=%11$.8f\tbid/ask=%12$.8f/%13$.8f"
        "\tpos=%14%\torder=%15%/%16%/-",
        [&](TradingRecord &record) {
          record % eventDesc                                           // 1
              % m_self.GetCloseOrderSide()                             // 2
              % m_security.GetSymbol().GetSymbol().c_str()             // 3
              % m_self.GetTradingSystem().GetInstanceName().c_str()    // 4
              % m_tradingSystem.GetMode()                              // 5
              % m_closeReason                                          // 6
              % m_security.DescalePrice(m_self.GetCloseStartPrice());  // 7
          const auto &order = m_close.orders.back();
          if (order.price) {
            record % m_security.DescalePrice(*order.price);  // 8
          } else {
            record % '-';  // 8
          }
          record % m_self.GetCurrency()        // 9
              % maxQty                         // 10
              % m_self.GetActiveQty()          // 11
              % m_security.GetBidPriceValue()  // 12
              % m_security.GetAskPriceValue()  // 13
              % m_operationId                  // 14
              % uuid;                          // 15
          if (id) {
            record % *id;  // 16 and last
          } else {
            record % '-';  // 16 and last
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
        "\tpos=%17%\torder=%18%/%19%/%20%",
        [this, &tsOrderId, &orderStatus](TradingRecord &record) {
          const auto &order = m_close.orders.back();
          record % orderStatus                                         // 1
              % m_self.GetCloseOrderSide()                             // 2
              % m_security.GetSymbol().GetSymbol().c_str()             // 3
              % m_self.GetTradingSystem().GetInstanceName().c_str()    // 4
              % m_tradingSystem.GetMode()                              // 5
              % m_closeReason                                          // 6
              % m_security.DescalePrice(m_self.GetCloseStartPrice());  // 7
          if (order.price) {
            record % m_security.DescalePrice(*order.price);  // 8
          } else {
            record % '-';  // 8
          }
          if (m_self.GetClosedQty()) {
            record % m_security.DescalePrice(m_close.lastTradePrice)   // 9
                % m_security.DescalePrice(m_self.GetCloseAvgPrice());  // 10
          } else {
            record % '-' % '-';  // 9, 10
          }
          record % m_self.GetCurrency()        // 11
              % m_self.GetOpenedQty()          // 12
              % m_self.GetClosedQty()          // 13
              % m_self.GetActiveQty()          // 14
              % m_security.GetBidPriceValue()  // 15
              % m_security.GetAskPriceValue()  // 16
              % m_operationId                  // 17
              % order.uuid                     // 18
              % order.id                       // 19
              % tsOrderId;                     // 20 and last
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

    m_open.orders.emplace_back(std::move(now), boost::none, m_planedQty,
                               generateUuid());
    try {
      auto &order = m_open.orders.back();
      order.isActive = false;

      m_open.time = order.time;
      m_open.OnNewTrade(TradingSystem::TradeInfo{
          "", order.qty, m_security.ScalePrice(openPrice)});

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
  OrderId Open(const OpenImpl &openImpl,
               const TimeInForce &timeInForce,
               const OrderParams &orderParams,
               boost::optional<ScaledPrice> &&price) {
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

    Assert(!m_self.IsOpened());
    Assert(!m_self.IsError());
    Assert(!m_self.IsClosed());

    AssertGt(m_planedQty, m_open.qty);
    auto qty = m_planedQty - m_open.qty;

    if (!m_isRegistered) {
      m_strategy.Register(m_self);
      // supporting prev. logic (when was m_strategy = nullptr),
      // don't know why set flag in other place.
    }

    m_open.orders.emplace_back(std::move(now), std::move(price), qty,
                               generateUuid());
    ReportOpeningStart("start", boost::none);
    auto &order = m_open.orders.back();

    try {
      if (!orderParams.expiration && !m_security.GetSymbol().IsExplicit()) {
        m_expiration = m_security.GetExpiration();
        OrderParams additionalOrderParams(orderParams);
        additionalOrderParams.expiration = &*m_expiration;
        order.id = openImpl(qty, additionalOrderParams);
      } else {
        order.id = openImpl(qty, orderParams);
      }

      ReportOpeningStart("sent", order.id);

      m_isRegistered = true;  // supporting prev. logic
                              // (when was m_strategy = nullptr),
                              // don't know why set flag only here.

    } catch (...) {
      if (m_isRegistered) {
        m_strategy.Unregister(m_self);
      }
      try {
        CopyOrder(nullptr,  // order ID (from trading system)
                  true, ORDER_STATUS_ERROR, &timeInForce, &orderParams);
      } catch (...) {
        AssertFailNoException();
      }
      m_open.orders.pop_back();
      throw;
    }

    try {
      CopyOrder(nullptr,  // order ID (from trading system)
                true, ORDER_STATUS_SENT, &timeInForce, &orderParams);
    } catch (...) {
      AssertFailNoException();
    }

    return order.id;
  }

  template <typename CloseImpl>
  OrderId Close(const CloseImpl &closeImpl,
                const TimeInForce &timeInForce,
                boost::optional<ScaledPrice> &&price,
                const Qty &maxQty,
                const OrderParams &orderParams) {
    return Close(closeImpl, timeInForce, std::move(price), maxQty, orderParams,
                 generateUuid());
  }

  template <typename CloseImpl>
  OrderId Close(const CloseImpl &closeImpl,
                const TimeInForce &timeInForce,
                boost::optional<ScaledPrice> &&price,
                const Qty &maxQty,
                const OrderParams &orderParams,
                const uuids::uuid &&uuid) {
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
                                std::min<Qty>(maxQty, m_self.GetActiveQty()),
                                std::move(uuid));
    Order &order = m_close.orders.back();

    ReportClosingStart("start", order.uuid, boost::none, maxQty);

    try {
      if (m_expiration && !orderParams.expiration) {
        OrderParams additionalOrderParams(orderParams);
        additionalOrderParams.expiration = &*m_expiration;
        order.id = closeImpl(order.qty, additionalOrderParams);
      } else {
        order.id = closeImpl(order.qty, orderParams);
      }

      ReportClosingStart("sent", order.uuid, order.id, maxQty);

    } catch (...) {
      CopyOrder(nullptr,  // order ID (from trading system)
                false, ORDER_STATUS_SENT, &timeInForce, &orderParams);
      m_close.orders.pop_back();
      throw;
    }

    try {
      CopyOrder(nullptr,  // order ID (from trading system)
                false, ORDER_STATUS_SENT, &timeInForce, &orderParams);
    } catch (...) {
      AssertFailNoException();
    }

    return order.id;
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
        "\tpos=%5%\torder=%6%/%7%",
        [this, &order, &direction](TradingRecord &record) {
          record % direction                                         // 1
              % m_security.GetSymbol().GetSymbol().c_str()           // 2
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 3
              % m_tradingSystem.GetMode()                            // 4
              % m_operationId                                        // 5
              % order.uuid                                           // 6
              % order.id;                                            // 7
        });
    m_tradingSystem.CancelOrder(order.id);
    order.isCanceled = true;
    return true;
  }

 private:
  void CopyOrder(const std::string *orderTradeSystemId,
                 bool isOpen,
                 const OrderStatus &status,
                 const TimeInForce *timeInForce = nullptr,
                 const OrderParams *orderParams = nullptr) {
    Assert(!m_open.orders.empty());
    Assert(isOpen || !m_close.orders.empty());

    m_strategy.GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {

      const Order &order =
          isOpen ? m_open.orders.back() : m_close.orders.back();

      double orderPrice;
      if (order.price) {
        orderPrice = m_security.DescalePrice(*order.price);
      }

      pt::ptime execTime;
      double bidPrice;
      Qty bidQty;
      double askPrice;
      Qty askQty;
      if (!orderTradeSystemId) {
        bidPrice = m_security.GetBidPriceValue();
        bidQty = m_security.GetBidQtyValue();
        askPrice = m_security.GetAskPriceValue();
        askQty = m_security.GetAskQtyValue();
      }
      static_assert(numberOfOrderStatuses == 9, "List changed");
      switch (status) {
        case ORDER_STATUS_CANCELLED:
        case ORDER_STATUS_FILLED:
        case ORDER_STATUS_FILLED_PARTIALLY:
        case ORDER_STATUS_REJECTED:
        case ORDER_STATUS_INACTIVE:
        case ORDER_STATUS_ERROR:
          execTime = m_strategy.GetContext().GetCurrentTime();
          break;
      }

      dropCopy.CopyOrder(
          order.uuid, orderTradeSystemId, order.time,
          !execTime.is_not_a_date_time() ? &execTime : nullptr, status,
          m_operationId, &m_subOperationId, m_security, m_tradingSystem,
          m_self.GetType() == TYPE_LONG
              ? (isOpen ? ORDER_SIDE_BUY : ORDER_SIDE_SELL)
              : (!isOpen ? ORDER_SIDE_BUY : ORDER_SIDE_SELL),
          order.qty, order.price ? &orderPrice : nullptr, timeInForce,
          m_self.GetCurrency(),
          orderParams && orderParams->minTradeQty ? &*orderParams->minTradeQty
                                                  : nullptr,
          order.executedQty, !orderTradeSystemId ? &bidPrice : nullptr,
          !orderTradeSystemId ? &bidQty : nullptr,
          !orderTradeSystemId ? &askPrice : nullptr,
          !orderTradeSystemId ? &askQty : nullptr);

    });
  }

  void CopyTrade(const std::string &tradingSystemId,
                 double price,
                 const Qty &qty,
                 bool isOpen) {
    Assert(!m_open.orders.empty());
    Assert(isOpen || !m_close.orders.empty());

    m_strategy.GetContext().InvokeDropCopy([&](DropCopy &dropCopy) {

      const Order &order =
          isOpen ? m_open.orders.back() : m_close.orders.back();

      dropCopy.CopyTrade(m_strategy.GetContext().GetCurrentTime(),
                         tradingSystemId, order.uuid, price, qty,
                         m_security.GetBidPrice(), m_security.GetBidQty(),
                         m_security.GetAskPrice(), m_security.GetAskQty());

    });
  }
};

//////////////////////////////////////////////////////////////////////////

Position::Position(Strategy &strategy,
                   const uuids::uuid &operationId,
                   int64_t subOperationId,
                   TradingSystem &tradingSystem,
                   Security &security,
                   const Currency &currency,
                   const Qty &qty,
                   const ScaledPrice &startPrice,
                   const TimeMeasurement::Milestones &timeMeasurement)
    : m_pimpl(std::make_unique<Implementation>(*this,
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

#pragma warning(push)
#pragma warning(disable : 4702)
Position::Position() {
  AssertFail("Position::Position exists only for virtual inheritance");
  throw LogicError("Position::Position exists only for virtual inheritance");
}
#pragma warning(pop)

Position::~Position() {}

const uuids::uuid &Position::GetId() const { return m_pimpl->m_operationId; }

bool Position::IsLong() const {
  static_assert(numberOfTypes == 2, "List changed.");
  Assert(GetType() == Position::TYPE_LONG || GetType() == Position::TYPE_SHORT);
  return GetType() == Position::TYPE_LONG;
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

void Position::SetCloseReason(const CloseReason &newCloseReason) noexcept {
  AssertNe(CLOSE_REASON_NONE, newCloseReason);
  if (m_pimpl->m_closeReason != CLOSE_REASON_NONE) {
    return;
  }
  m_pimpl->m_closeReason = newCloseReason;
}

void Position::ResetCloseReason(const CloseReason &newReason) noexcept {
  m_pimpl->m_closeReason = newReason;
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
                             const TradingSystem::TradeInfo *trade) {
  m_pimpl->UpdateOpening(orderId, tradingSystemOrderId, orderStatus,
                         remainingQty, trade);
}

void Position::UpdateClosing(const OrderId &orderId,
                             const std::string &tradingSystemOrderId,
                             const OrderStatus &orderStatus,
                             const Qty &remainingQty,
                             const TradingSystem::TradeInfo *trade) {
  m_pimpl->UpdateClosing(orderId, tradingSystemOrderId, orderStatus,
                         remainingQty, trade);
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
  return RoundByScale(activeQty * (GetOpenedVolume() / m_pimpl->m_open.qty),
                      GetSecurity().GetPriceScale());
}

const Qty &Position::GetClosedQty() const noexcept {
  return m_pimpl->m_close.qty;
}

Volume Position::GetClosedVolume() const {
  return m_pimpl->m_security.DescalePrice(m_pimpl->m_close.volume);
}

const pt::ptime &Position::GetCloseTime() const {
  return m_pimpl->m_close.time;
}

const ScaledPrice &Position::GetActiveOrderPrice() const {
  Assert(!m_pimpl->m_open.HasActiveOrders() ||
         !m_pimpl->m_close.HasActiveOrders());
  if (m_pimpl->m_close.HasActiveOrders()) {
    Assert(!m_pimpl->m_open.HasActiveOrders());
    const auto &price = m_pimpl->m_close.orders.back().price;
    if (!price) {
      throw Exception(
          "Position current active close-order is order without price");
    }
    return *price;
  } else if (m_pimpl->m_open.HasActiveOrders()) {
    const auto &price = m_pimpl->m_open.orders.back().price;
    if (!price) {
      throw Exception(
          "Position current active open-order is order without price");
    }
    return *price;
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

const ScaledPrice &Position::GetLastTradePrice() const {
  if (m_pimpl->m_close.numberOfTrades) {
    return m_pimpl->m_close.lastTradePrice;
  } else if (m_pimpl->m_open.numberOfTrades) {
    return m_pimpl->m_open.lastTradePrice;
  } else {
    throw Exception("Position has no trades");
  }
}

Volume Position::GetRealizedPnlVolume() const {
  return RoundByScale(GetRealizedPnl() * GetSecurity().GetQuoteSize(),
                      GetSecurity().GetPriceScale());
}

Double Position::GetRealizedPnlPercentage() const {
  const auto ratio = GetRealizedPnlRatio();
  const auto result = ratio >= 1 ? ratio - 1 : -(1 - ratio);
  return result * 100;
}

Volume Position::GetPlannedPnl() const {
  return RoundByScale(GetUnrealizedPnl() + GetRealizedPnl(),
                      GetSecurity().GetPriceScale());
}

bool Position::IsProfit() const {
  const auto ratio = GetRealizedPnlRatio();
  return ratio > 1.0 && !IsEqual(ratio, 1.0);
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
  m_pimpl->m_algos.emplace_back(std::move(algo));
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

const ScaledPrice &Position::GetOpenStartPrice() const {
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

ScaledPrice Position::GetOpenAvgPrice() const {
  if (!m_pimpl->m_open.qty) {
    throw Exception("Position has no open price");
  }
  return ScaledPrice(m_pimpl->m_open.volume / m_pimpl->m_open.qty);
}

const ScaledPrice &Position::GetActiveOpenOrderPrice() const {
  Assert(!m_pimpl->m_open.HasActiveOrders() ||
         !m_pimpl->m_close.HasActiveOrders());
  if (!m_pimpl->m_open.HasActiveOrders()) {
    throw Exception("Position has no active open-order");
  }
  Assert(!m_pimpl->m_close.HasActiveOrders());
  const auto &price = m_pimpl->m_open.orders.back().price;
  if (!price) {
    throw Exception(
        "Position current active open-order is order without price");
  }
  AssertEq(*price, GetActiveOrderPrice());
  return *price;
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

const ScaledPrice &Position::GetLastOpenTradePrice() const {
  if (!m_pimpl->m_open.numberOfTrades) {
    throw Exception("Position has no open trades");
  }
  return m_pimpl->m_open.lastTradePrice;
}

Volume Position::GetOpenedVolume() const {
  return m_pimpl->m_security.DescalePrice(m_pimpl->m_open.volume);
}

const ScaledPrice &Position::GetCloseStartPrice() const {
  return m_pimpl->m_close.startPrice;
}

const pt::ptime &Position::GetCloseStartTime() const {
  if (m_pimpl->m_close.orders.empty()) {
    throw Exception("Position has no close-order");
  }
  return m_pimpl->m_close.orders.front().time;
}

void Position::SetCloseStartPrice(const ScaledPrice &price) {
  m_pimpl->m_close.startPrice = price;
}

ScaledPrice Position::GetCloseAvgPrice() const {
  if (!m_pimpl->m_close.qty) {
    throw Exception("Position has no close price");
  }
  return ScaledPrice(m_pimpl->m_close.volume / m_pimpl->m_close.qty);
}

const ScaledPrice &Position::GetActiveCloseOrderPrice() const {
  Assert(!m_pimpl->m_open.HasActiveOrders() ||
         !m_pimpl->m_close.HasActiveOrders());
  if (!m_pimpl->m_close.HasActiveOrders()) {
    throw Exception("Position has no active close-order");
  }
  Assert(!m_pimpl->m_open.HasActiveOrders());
  const auto &price = m_pimpl->m_close.orders.back().price;
  if (!price) {
    throw Exception(
        "Position current active close-order is order without price");
  }
  AssertEq(*price, GetActiveOrderPrice());
  return *price;
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

const ScaledPrice &Position::GetLastCloseTradePrice() const {
  if (!m_pimpl->m_close.numberOfTrades) {
    throw Exception("Position has no close trades");
  }
  return m_pimpl->m_close.lastTradePrice;
}

void Position::RestoreOpenState(const trdk::Price &openPrice) {
  m_pimpl->RestoreOpenState(openPrice);
}

OrderId Position::OpenAtMarketPrice() {
  return OpenAtMarketPrice(defaultOrderParams);
}

OrderId Position::OpenAtMarketPrice(const OrderParams &params) {
  return m_pimpl->Open(
      [this](const Qty &qty, const OrderParams &params) {
        return DoOpenAtMarketPrice(qty, params);
      },
      TIME_IN_FORCE_GTC, params, boost::none);
}

OrderId Position::Open(const ScaledPrice &price) {
  return Open(price, defaultOrderParams);
}

OrderId Position::Open(const ScaledPrice &price, const OrderParams &params) {
  return m_pimpl->Open(
      [this, &price](const Qty &qty, const OrderParams &params) {
        return DoOpen(qty, price, params);
      },
      TIME_IN_FORCE_GTC, params, price);
}

OrderId Position::OpenAtMarketPriceWithStopPrice(const ScaledPrice &stopPrice) {
  return OpenAtMarketPriceWithStopPrice(stopPrice, defaultOrderParams);
}

OrderId Position::OpenAtMarketPriceWithStopPrice(const ScaledPrice &stopPrice,
                                                 const OrderParams &params) {
  return m_pimpl->Open(
      [this, stopPrice](const Qty &qty, const OrderParams &params) {
        return DoOpenAtMarketPriceWithStopPrice(qty, stopPrice, params);
      },
      TIME_IN_FORCE_GTC, params, boost::none);
}

OrderId Position::OpenImmediatelyOrCancel(const ScaledPrice &price) {
  return OpenImmediatelyOrCancel(price, defaultOrderParams);
}

OrderId Position::OpenImmediatelyOrCancel(const ScaledPrice &price,
                                          const OrderParams &params) {
  return m_pimpl->Open(
      [this, price](const Qty &qty, const OrderParams &params) {
        return DoOpenImmediatelyOrCancel(qty, price, params);
      },
      TIME_IN_FORCE_IOC, params, price);
}

OrderId Position::OpenAtMarketPriceImmediatelyOrCancel() {
  return OpenAtMarketPriceImmediatelyOrCancel(defaultOrderParams);
}

OrderId Position::OpenAtMarketPriceImmediatelyOrCancel(
    const OrderParams &params) {
  return m_pimpl->Open(
      [this](const Qty &qty, const OrderParams &params) {
        return DoOpenAtMarketPriceImmediatelyOrCancel(qty, params);
      },
      TIME_IN_FORCE_IOC, params, boost::none);
}

OrderId Position::CloseAtMarketPrice() {
  return CloseAtMarketPrice(defaultOrderParams);
}

OrderId Position::CloseAtMarketPrice(const OrderParams &params) {
  return m_pimpl->Close(
      [this](const Qty &qty, const OrderParams &params) {
        return DoCloseAtMarketPrice(qty, params);
      },
      TIME_IN_FORCE_GTC, boost::none, GetActiveQty(), params);
}

OrderId Position::Close(const ScaledPrice &price) {
  return Close(price, GetActiveQty(), defaultOrderParams);
}

OrderId Position::Close(const ScaledPrice &price, const Qty &maxQty) {
  return Close(price, maxQty, defaultOrderParams);
}

OrderId Position::Close(const ScaledPrice &price, const OrderParams &params) {
  return Close(price, GetActiveQty(), params);
}

OrderId Position::Close(const ScaledPrice &price,
                        const Qty &maxQty,
                        const OrderParams &params) {
  return m_pimpl->Close(
      [this, &price](const Qty &qty, const OrderParams &params) {
        return DoClose(qty, price, params);
      },
      TIME_IN_FORCE_GTC, price, maxQty, params);
}

OrderId Position::CloseAtMarketPriceWithStopPrice(
    const ScaledPrice &stopPrice) {
  return CloseAtMarketPriceWithStopPrice(stopPrice, defaultOrderParams);
}

OrderId Position::CloseAtMarketPriceWithStopPrice(const ScaledPrice &stopPrice,
                                                  const OrderParams &params) {
  return m_pimpl->Close(
      [this, stopPrice](const Qty &qty, const OrderParams &params) {
        return DoCloseAtMarketPriceWithStopPrice(qty, stopPrice, params);
      },
      TIME_IN_FORCE_GTC, boost::none, GetActiveQty(), params);
}

OrderId Position::CloseImmediatelyOrCancel(const ScaledPrice &price) {
  return CloseImmediatelyOrCancel(price, defaultOrderParams);
}

OrderId Position::CloseImmediatelyOrCancel(const ScaledPrice &price,
                                           const OrderParams &params) {
  return m_pimpl->Close(
      [this, price](const Qty &qty, const OrderParams &params) {
        return DoCloseImmediatelyOrCancel(qty, price, params);
      },
      TIME_IN_FORCE_IOC, price, GetActiveQty(), params);
}

OrderId Position::CloseAtMarketPriceImmediatelyOrCancel() {
  return CloseAtMarketPriceImmediatelyOrCancel(defaultOrderParams);
}

OrderId Position::CloseAtMarketPriceImmediatelyOrCancel(
    const OrderParams &params) {
  return m_pimpl->Close(
      [this](const Qty &qty, const OrderParams &params) {
        return DoCloseAtMarketPriceImmediatelyOrCancel(qty, params);
      },
      TIME_IN_FORCE_IOC, boost::none, GetActiveQty(), params);
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
                           const ScaledPrice &startPrice,
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
            % GetSecurity().DescalePrice(GetOpenStartPrice())   // 4
            % GetCurrency()                                     // 5
            % GetPlanedQty()                                    // 6
            % GetId();                                          // 7 and last
      });
}

LongPosition::LongPosition() {
  GetStrategy().GetTradingLog().Write(
      "position\tnew\tlong\t%1%\t%2%.%3%\tprice=%4$.8f\t%5%\tqty=%6$.8f"
      "\tpos=%7%",
      [this](TradingRecord &record) {
        record % GetSecurity().GetSymbol().GetSymbol().c_str()  // 1
            % GetTradingSystem().GetInstanceName().c_str()      // 2
            % GetTradingSystem().GetMode()                      // 3
            % GetSecurity().DescalePrice(GetOpenStartPrice())   // 4
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
    return RoundByScale(GetClosedVolume() - GetOpenedVolume(),
                        GetSecurity().GetPriceScale());
  }
  const auto openedVolume = (GetOpenedQty() - activeQty) *
                            GetSecurity().DescalePrice(GetOpenAvgPrice());
  return RoundByScale(GetClosedVolume() - openedVolume,
                      GetSecurity().GetPriceScale());
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
  const auto openedVolume =
      (GetOpenedQty() - activeQty) * GetSecurity().DescalePrice(openPrice);
  return GetClosedVolume() / openedVolume;
}

Volume LongPosition::GetUnrealizedPnl() const {
  return RoundByScale(
      (GetActiveQty() * GetSecurity().GetBidPrice()) - GetActiveVolume(),
      GetSecurity().GetPriceScale());
}

ScaledPrice LongPosition::GetMarketOpenPrice() const {
  return GetSecurity().GetAskPriceScaled();
}

ScaledPrice LongPosition::GetMarketClosePrice() const {
  return GetSecurity().GetBidPriceScaled();
}

ScaledPrice LongPosition::GetMarketOpenOppositePrice() const {
  return GetSecurity().GetBidPriceScaled();
}

ScaledPrice LongPosition::GetMarketCloseOppositePrice() const {
  return GetSecurity().GetAskPriceScaled();
}

OrderId LongPosition::DoOpenAtMarketPrice(const Qty &qty,
                                          const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().BuyAtMarketPrice(
      GetSecurity(), GetCurrency(), qty, params,
      boost::bind(&LongPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId LongPosition::DoOpen(const Qty &qty,
                             const ScaledPrice &price,
                             const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().Buy(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&LongPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId LongPosition::DoOpenAtMarketPriceWithStopPrice(
    const Qty &qty, const ScaledPrice &stopPrice, const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().BuyAtMarketPriceWithStopPrice(
      GetSecurity(), GetCurrency(), qty, stopPrice, params,
      boost::bind(&LongPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId LongPosition::DoOpenImmediatelyOrCancel(const Qty &qty,
                                                const ScaledPrice &price,
                                                const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().BuyImmediatelyOrCancel(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&LongPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId LongPosition::DoOpenAtMarketPriceImmediatelyOrCancel(
    const Qty &qty, const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().BuyAtMarketPriceImmediatelyOrCancel(
      GetSecurity(), GetCurrency(), qty, params,
      boost::bind(&LongPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId LongPosition::DoCloseAtMarketPrice(const Qty &qty,
                                           const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SellAtMarketPrice(
      GetSecurity(), GetCurrency(), qty, params,
      boost::bind(&LongPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId LongPosition::DoClose(const Qty &qty,
                              const ScaledPrice &price,
                              const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().Sell(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&LongPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId LongPosition::DoCloseAtMarketPriceWithStopPrice(
    const Qty &qty, const ScaledPrice &stopPrice, const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SellAtMarketPriceWithStopPrice(
      GetSecurity(), GetCurrency(), qty, stopPrice, params,
      boost::bind(&LongPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId LongPosition::DoCloseImmediatelyOrCancel(const Qty &qty,
                                                 const ScaledPrice &price,
                                                 const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  AssertLt(0, qty);
  return GetTradingSystem().SellImmediatelyOrCancel(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&LongPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId LongPosition::DoCloseAtMarketPriceImmediatelyOrCancel(
    const Qty &qty, const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  AssertLt(0, qty);
  return GetTradingSystem().SellAtMarketPriceImmediatelyOrCancel(
      GetSecurity(), GetCurrency(), qty, params,
      boost::bind(&LongPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(Strategy &strategy,
                             const uuids::uuid &operationId,
                             int64_t subOperationId,
                             TradingSystem &tradingSystem,
                             Security &security,
                             const Currency &currency,
                             const Qty &qty,
                             const ScaledPrice &startPrice,
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
            % GetSecurity().DescalePrice(GetOpenStartPrice())   // 4
            % GetCurrency()                                     // 5
            % GetPlanedQty()                                    // 6
            % GetId();                                          // 7 and last
      });
}

ShortPosition::ShortPosition() {
  GetStrategy().GetTradingLog().Write(
      "position\tnew\tshort\t%1%\t%2%.%3%\tprice=%4$.8f\t%5%\tqty=%6$.8f"
      "\tpos=%7%",
      [this](TradingRecord &record) {
        record % GetSecurity().GetSymbol().GetSymbol().c_str()  // 1
            % GetTradingSystem().GetInstanceName().c_str()      // 2
            % GetTradingSystem().GetMode()                      // 3
            % GetSecurity().DescalePrice(GetOpenStartPrice())   // 4
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
    return RoundByScale(GetOpenedVolume() - GetClosedVolume(),
                        GetSecurity().GetPriceScale());
  }
  const auto openedVolume = (GetOpenedQty() - GetActiveQty()) *
                            GetSecurity().DescalePrice(GetOpenAvgPrice());
  return RoundByScale(openedVolume - GetClosedVolume(),
                      GetSecurity().GetPriceScale());
}
Double ShortPosition::GetRealizedPnlRatio() const {
  const auto closedVolume = GetClosedVolume();
  if (closedVolume == 0) {
    return 0;
  }
  if (GetActiveQty() == 0) {
    return GetOpenedVolume() / closedVolume;
  }
  const auto openedVolume = (GetOpenedQty() - GetActiveQty()) *
                            GetSecurity().DescalePrice(GetOpenAvgPrice());
  return openedVolume / closedVolume;
}
Volume ShortPosition::GetUnrealizedPnl() const {
  return RoundByScale(
      GetActiveVolume() - (GetActiveQty() * GetSecurity().GetAskPrice()),
      GetSecurity().GetPriceScale());
}

ScaledPrice ShortPosition::GetMarketOpenPrice() const {
  return GetSecurity().GetBidPriceScaled();
}

ScaledPrice ShortPosition::GetMarketClosePrice() const {
  return GetSecurity().GetAskPriceScaled();
}

ScaledPrice ShortPosition::GetMarketOpenOppositePrice() const {
  return GetSecurity().GetAskPriceScaled();
}

ScaledPrice ShortPosition::GetMarketCloseOppositePrice() const {
  return GetSecurity().GetBidPriceScaled();
}

OrderId ShortPosition::DoOpenAtMarketPrice(const Qty &qty,
                                           const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SellAtMarketPrice(
      GetSecurity(), GetCurrency(), qty, params,
      boost::bind(&ShortPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId ShortPosition::DoOpen(const Qty &qty,
                              const ScaledPrice &price,
                              const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().Sell(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&ShortPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId ShortPosition::DoOpenAtMarketPriceWithStopPrice(
    const Qty &qty, const ScaledPrice &stopPrice, const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SellAtMarketPriceWithStopPrice(
      GetSecurity(), GetCurrency(), qty, stopPrice, params,
      boost::bind(&ShortPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId ShortPosition::DoOpenImmediatelyOrCancel(const Qty &qty,
                                                 const ScaledPrice &price,
                                                 const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SellImmediatelyOrCancel(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&ShortPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId ShortPosition::DoOpenAtMarketPriceImmediatelyOrCancel(
    const Qty &qty, const OrderParams &params) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SellAtMarketPriceImmediatelyOrCancel(
      GetSecurity(), GetCurrency(), qty, params,
      boost::bind(&ShortPosition::UpdateOpening, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId ShortPosition::DoCloseAtMarketPrice(const Qty &qty,
                                            const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().BuyAtMarketPrice(
      GetSecurity(), GetCurrency(), qty, params,
      boost::bind(&ShortPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId ShortPosition::DoClose(const Qty &qty,
                               const ScaledPrice &price,
                               const OrderParams &params) {
  return GetTradingSystem().Buy(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&ShortPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId ShortPosition::DoCloseAtMarketPriceWithStopPrice(
    const Qty &qty, const ScaledPrice &stopPrice, const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().BuyAtMarketPriceWithStopPrice(
      GetSecurity(), GetCurrency(), qty, stopPrice, params,
      boost::bind(&ShortPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId ShortPosition::DoCloseImmediatelyOrCancel(const Qty &qty,
                                                  const ScaledPrice &price,
                                                  const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().BuyImmediatelyOrCancel(
      GetSecurity(), GetCurrency(), qty, price, params,
      boost::bind(&ShortPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

OrderId ShortPosition::DoCloseAtMarketPriceImmediatelyOrCancel(
    const Qty &qty, const OrderParams &params) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().BuyAtMarketPriceImmediatelyOrCancel(
      GetSecurity(), GetCurrency(), qty, params,
      boost::bind(&ShortPosition::UpdateClosing, shared_from_this(), _1, _2, _3,
                  _4, _5),
      GetStrategy().GetRiskControlScope(), GetTimeMeasurement());
}

//////////////////////////////////////////////////////////////////////////
