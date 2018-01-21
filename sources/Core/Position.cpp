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
#include "Operation.hpp"
#include "OrderStatusHandler.hpp"
#include "Settings.hpp"
#include "Strategy.hpp"
#include "Timer.hpp"
#include "Trade.hpp"
#include "TradingLog.hpp"
#include "TradingSystem.hpp"
#include "TransactionContext.hpp"
#include "Common/ExpirationCalendar.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

namespace pt = boost::posix_time;
namespace sig = boost::signals2;

////////////////////////////////////////////////////////////////////////////////

namespace {
boost::atomic_size_t objectsCounter(0);

void ReportAboutGeneralAction(const Position &position,
                              const char *action,
                              const char *status) noexcept {
  position.GetStrategy().GetTradingLog().Write(
      "{'position': {'%1%': {'status': '%2%'}, 'startPrice': %3$.8f, 'qty': "
      "%4$.8f, 'type': '%5%', 'security': '%6%', 'operation': '%7%/%8%'}}",
      [&](TradingRecord &record) {
        record % action                         // 1
            % status                            // 2
            % position.GetOpenStartPrice()      // 3
            % position.GetPlanedQty()           // 4
            % position.GetSide()                // 5
            % position.GetSecurity()            // 6
            % position.GetOperation()->GetId()  // 7
            % position.GetSubOperationId();     // 8
      });
}

void ReportAboutDeletion(const Position &position) noexcept {
  position.GetStrategy().GetTradingLog().Write(
      "{'position': {'del': {'numberOfObjects': %1%}, 'startPrice': %2$.8f, "
      "'qty': %3$.8f, 'type': '%4%', 'security': '%5%', 'operation': "
      "'%6%/%7%'}}",
      [&](TradingRecord &record) {
        record % objectsCounter                 // 1
            % position.GetOpenStartPrice()      // 2
            % position.GetPlanedQty()           // 3
            % position.GetSide()                // 4
            % position.GetSecurity()            // 5
            % position.GetOperation()->GetId()  // 6
            % position.GetSubOperationId();     // 7
      });
}

enum OrderStatusStep {
  ORDER_STATUS_STEP_SENT,
  ORDER_STATUS_STEP_OPENED,
  ORDER_STATUS_STEP_CLOSED,
};
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

    OrderStatusStep status;
    bool isCanceled;
    bool isRejected;

    const boost::optional<Price> price;
    const Qty qty;

    boost::shared_ptr<const OrderTransactionContext> transactionContext;

    Qty executedQty;

    Volume commission;

    explicit Order(const pt::ptime &&time,
                   boost::optional<Price> &&price,
                   const Qty &qty)
        : time(std::move(time)),
          status(ORDER_STATUS_STEP_SENT),
          isCanceled(false),
          isRejected(false),
          price(std::move(price)),
          qty(qty),
          executedQty(0),
          commission(0) {}
  };

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
      return !orders.empty() &&
             orders.back().status <= ORDER_STATUS_STEP_OPENED;
    }
    bool HasOpenedOrders() const {
      return !orders.empty() &&
             orders.back().status == ORDER_STATUS_STEP_OPENED;
    }

    bool IsCanceling() const {
      return HasActiveOrders() && orders.back().isCanceled;
    }

    void AddTrade(const Trade &trade) { AddTrade(trade.qty, trade.price); }
    void AddTrade(const Qty &tradeQty, const Price &tradePrice) {
      AssertLt(0, tradePrice);
      AssertLt(0, tradeQty);
      Assert(!orders.empty());
      auto &order = orders.back();
      order.executedQty += tradeQty;
      volume += tradePrice * tradeQty;
      qty += tradeQty;
      ++numberOfTrades;
      lastTradePrice = tradePrice;
    }
    bool CheckAndAddVirtualTrade(const Qty &tradeQty, const Price &tradePrice) {
      if (orders.empty()) {
        return false;
      }
      auto &order = orders.back();
      if (order.executedQty + tradeQty > order.qty) {
        AssertLe(order.executedQty + tradeQty, order.qty);
        throw Exception(
            "Virtual trade quantity is greater than active order remaining "
            "quantity");
      }
      AddTrade(tradeQty, tradePrice);
      return true;
    }
  };

  class StatusHandler : public OrderStatusHandler {
   public:
    explicit StatusHandler(boost::shared_ptr<Position> &&position)
        : m_position(std::move(position)) {}

    virtual ~StatusHandler() override = default;

   public:
    virtual void OnOpen() override {
      const auto lock = Lock();
      Assert(!m_position->IsClosed());
      Assert(!m_position->IsCompleted());
      auto &order = GetOrder();
      AssertEq(0, order.executedQty);
      AssertEq(ORDER_STATUS_STEP_SENT, order.status);
      order.status = ORDER_STATUS_STEP_OPENED;
      Report(ORDER_STATUS_OPENED);
    }

    virtual void OnCancel() override {
      auto lock = Lock();
      Assert(!m_position->IsClosed());
      Assert(!m_position->IsCompleted());
      auto &order = GetOrder();
      AssertLe(order.executedQty, order.qty);
      AssertGe(ORDER_STATUS_STEP_OPENED, order.status);
      order.status = ORDER_STATUS_STEP_CLOSED;
      UpdateStat();
      Report(ORDER_STATUS_CANCELED);
      SignalUpdate(lock);
    }

    virtual void OnTrade(const Trade &trade, bool isFull) override {
      auto lock = Lock();
      Assert(!m_position->IsClosed());
      Assert(!m_position->IsCompleted());
      auto &order = GetOrder();
      AssertGe(ORDER_STATUS_STEP_OPENED, order.status);
      AssertLe(order.executedQty, order.qty);
      AssertLe(order.executedQty + trade.qty, order.qty);
      order.status = ORDER_STATUS_STEP_OPENED;
      GetDirection().AddTrade(trade);
      if (!GetPositionImpl().m_defaultOrderParams.position) {
        GetPositionImpl().m_defaultOrderParams.position =
            &*GetOrder().transactionContext;
      }
      if (!isFull) {
        Report(ORDER_STATUS_FILLED_PARTIALLY);
      } else {
        order.status = ORDER_STATUS_STEP_CLOSED;
        UpdateStat();
        Report(ORDER_STATUS_FILLED_FULLY);
        SignalUpdate(lock);
      }
    }

    virtual void OnReject() override {
      auto lock = Lock();
      Assert(!m_position->IsClosed());
      Assert(!m_position->IsCompleted());
      auto &order = GetOrder();
      AssertLe(order.executedQty, order.qty);
      AssertGe(ORDER_STATUS_STEP_OPENED, order.status);
      Assert(!order.isRejected);
      order.status = ORDER_STATUS_STEP_CLOSED;
      order.isRejected = true;
      UpdateStat();
      Report(ORDER_STATUS_REJECTED);
      SignalUpdate(lock);
    }

    virtual void OnError() override {
      auto lock = Lock();
      Assert(!m_position->IsClosed());
      Assert(!m_position->IsCompleted());
      auto &order = GetOrder();
      AssertLe(order.executedQty, order.qty);
      AssertGe(ORDER_STATUS_STEP_OPENED, order.status);
      order.status = ORDER_STATUS_STEP_CLOSED;
      GetPositionImpl().m_isError = true;
      UpdateStat();
      Report(ORDER_STATUS_ERROR);
      SignalUpdate(lock);
    }

    virtual void OnCommission(const Volume &commission) override {
      const auto lock = Lock();
      GetOrder().commission = commission;
    }

   protected:
    Module::Lock Lock() {
      return m_position->GetStrategy().LockForOtherThreads();
    }

    Position &GetPosition() { return *m_position; }
    const Position &GetPosition() const {
      return const_cast<StatusHandler *>(this)->GetPosition();
    }

    Implementation &GetPositionImpl() { return *m_position->m_pimpl; }
    const Implementation &GetPositionImpl() const {
      return const_cast<StatusHandler *>(this)->GetPositionImpl();
    }

    virtual DirectionData &GetDirection() = 0;

    Order &GetOrder() {
      auto &orders = GetDirection().orders;
      Assert(!orders.empty());
      return orders.back();
    }

    virtual const char *GetSide() const = 0;
    virtual const Qty &GetFilledQty() const = 0;

   private:
    void UpdateStat() {
      auto &order = GetOrder();
      AssertEq(ORDER_STATUS_STEP_CLOSED, order.status);
      AssertLe(order.executedQty, order.qty);
      if (!order.qty) {
        return;
      }
      auto &startTime = GetDirection().time;
      if (startTime != pt::not_a_date_time) {
        return;
      }
      startTime = GetPosition().GetSecurity().GetContext().GetCurrentTime();
    }

    void SignalUpdate(Module::Lock &lock) {
      lock.unlock();
      GetPositionImpl().SignalUpdate();
    }

    void Report(const OrderStatus &status) {
      GetPositionImpl().ReportAction(GetSide(), ConvertToPch(status),
                                     GetOrder(), GetFilledQty());
    }

   private:
    const boost::shared_ptr<Position> m_position;
  };

  class OpenStatusHandler : public StatusHandler {
   public:
    explicit OpenStatusHandler(boost::shared_ptr<Position> &&position)
        : StatusHandler(std::move(position)) {
      Assert(!GetPositionImpl().m_open.orders.empty());
    }

    virtual ~OpenStatusHandler() override = default;

   protected:
    virtual DirectionData &GetDirection() override {
      return GetPositionImpl().m_open;
    }
    virtual const char *GetSide() const override { return "open"; }
    virtual const Qty &GetFilledQty() const override {
      return GetPosition().GetOpenedQty();
    }
  };

  class CloseStatusHandler : public StatusHandler {
   public:
    explicit CloseStatusHandler(boost::shared_ptr<Position> &&position)
        : StatusHandler(std::move(position)) {
      Assert(!GetPositionImpl().m_close.orders.empty());
    }

    virtual ~CloseStatusHandler() override = default;

   protected:
    virtual DirectionData &GetDirection() override {
      return GetPositionImpl().m_close;
    }
    virtual const char *GetSide() const override { return "close"; }
    virtual const Qty &GetFilledQty() const override {
      return GetPosition().GetClosedQty();
    }
  };

 public:
  Position &m_self;

  const boost::shared_ptr<Operation> m_operation;

  TradingSystem *m_tradingSystem;

  mutable StateUpdateSignal m_stateUpdateSignal;

  Strategy &m_strategy;
  const int64_t m_subOperationId;
  bool m_isRegistered;
  Security *m_security;
  const Currency m_currency;

  Qty m_planedQty;

  boost::optional<ContractExpiration> m_expiration;

  bool m_isMarketAsCompleted;

  bool m_isError;

  TimeMeasurement::Milestones m_timeMeasurement;

  DirectionData m_open;
  DirectionData m_close;

  CloseReason m_closeReason;

  std::vector<boost::shared_ptr<Algo>> m_algos;

  OrderParams m_defaultOrderParams;

  Timer::Scope m_timerScope;

  explicit Implementation(Position &position,
                          const boost::shared_ptr<Operation> &operation,
                          TradingSystem &tradingSystem,
                          Strategy &strategy,
                          int64_t subOperationId,
                          Security &security,
                          const Currency &currency,
                          const Qty &qty,
                          const Price &startPrice,
                          const TimeMeasurement::Milestones &timeMeasurement)
      : m_self(position),
        m_operation(operation),
        m_tradingSystem(&tradingSystem),
        m_strategy(strategy),
        m_subOperationId(subOperationId),
        m_isRegistered(false),
        m_security(&security),
        m_currency(currency),
        m_planedQty(qty),
        m_isMarketAsCompleted(false),
        m_isError(false),
        m_timeMeasurement(timeMeasurement),
        m_open(startPrice),
        m_close(0),
        m_closeReason(CLOSE_REASON_NONE) {
    AssertLt(0, m_planedQty);
  }

 public:
  void SignalUpdate() { m_stateUpdateSignal(); }

 public:
  void ReportOpeningStateRestoration() const noexcept {
    Assert(!m_open.orders.empty());
    m_strategy.GetTradingLog().Write(
        "position\trestored\t%1%\t%2%\t%3%.%4%\topen_start_time=%5%\topen_time="
        "%6%\tprice=%7$.8f->%8$.8f\t%9%\tqty=%10$.8f\toperation=%11%/%12%",
        [this](TradingRecord &record) {
          const auto &order = m_open.orders.back();
          record % m_self.GetOpenOrderSide()                         // 1
              % m_security->GetSymbol().GetSymbol().c_str()          // 2
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 3
              % m_tradingSystem->GetMode()                           // 4
              % m_self.GetOpenStartPrice()                           // 5
              % m_self.GetOpenTime()                                 // 6
              % m_open.startPrice;                                   // 7
          if (order.price) {
            record % *order.price;  // 8
          } else {
            record % '-';  // 8
          }
          record % m_self.GetCurrency()  // 9
              % m_self.GetPlanedQty()    // 10
              % m_operation->GetId()     // 11
              % m_subOperationId;        // 12
        });
  }

  void ReportOpeningStart(const char *eventDesc,
                          const boost::optional<OrderId> &id) const noexcept {
    Assert(!m_open.orders.empty());
    m_strategy.GetTradingLog().Write(
        "position\topen->%1%\t%2%\t%3%\t%4%.%5%\tprice=%6$.8f->%7$.8f\t%8%"
        "\tqty=%9$.8f\tbid/ask=%10$.8f/%11$.8f\toperation=%12%/"
        "%13%\torder=%14%/-",
        [this, eventDesc, &id](TradingRecord &record) {
          const auto &order = m_open.orders.back();
          record % eventDesc                                         // 1
              % m_self.GetOpenOrderSide()                            // 2
              % m_security->GetSymbol().GetSymbol().c_str()          // 3
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 4
              % m_tradingSystem->GetMode()                           // 5
              % m_open.startPrice;                                   // 6
          if (order.price) {
            record % *order.price;  // 7
          } else {
            record % '-';  // 7
          }
          record % m_self.GetCurrency()         // 8
              % m_self.GetPlanedQty()           // 9
              % m_security->GetBidPriceValue()  // 10
              % m_security->GetAskPriceValue()  // 11
              % m_operation->GetId()            // 12
              % m_subOperationId;               // 13
          if (id) {
            record % *id;  // 14
          } else {
            record % '-';  // 14
          }
        });
  }

  void ReportClosingStart(const char *eventDesc,
                          const boost::optional<OrderId> &id,
                          const Qty &maxQty) const noexcept {
    m_strategy.GetTradingLog().Write(
        "position\tclose->%1%\t%2%\t%3%\t%4%.%5%"
        "\tclose_type=%6%\tprice=%7$.8f->%8$.8f\t%9%"
        "\tmax_qty=%10$.8f\tactive_qty=%11$.8f\tbid/ask=%12$.8f/%13$.8f"
        "\toperation=%14%/%15%\torder=%16%/-",
        [&](TradingRecord &record) {
          record % eventDesc                                         // 1
              % m_self.GetCloseOrderSide()                           // 2
              % m_security->GetSymbol().GetSymbol().c_str()          // 3
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 4
              % m_tradingSystem->GetMode()                           // 5
              % m_closeReason                                        // 6
              % m_self.GetCloseStartPrice();                         // 7
          const auto &order = m_close.orders.back();
          if (order.price) {
            record % *order.price;  // 8
          } else {
            record % '-';  // 8
          }
          record % m_self.GetCurrency()         // 9
              % maxQty                          // 10
              % m_self.GetActiveQty()           // 11
              % m_security->GetBidPriceValue()  // 12
              % m_security->GetAskPriceValue()  // 13
              % m_operation->GetId()            // 14
              % m_subOperationId;               // 15
          if (id) {
            record % *id;  // 16
          } else {
            record % '-';  // 16
          }
        });
  }

  void ReportAction(const char *action,
                    const char *status,
                    const Order &order,
                    const Qty &filledQty) {
    m_strategy.GetTradingLog().Write(
        "{'position': {'%1%': {'status': '%2%', 'orderPrice': %3$.8f, "
        "'lastPrice': %4$.8f, 'orderQty': %5$.8f, 'filledQty': %6$.8f, "
        "'remainingQty': %7$.8f, 'orderId': '%8%'}, 'startPrice': %9$.8f, "
        "'plannedQty': %10$.8f, 'activeQty': %11$.8f, 'type': '%12%', "
        "'security': '%13%', 'operation': '%14%/%15%'}}",
        [&](TradingRecord &record) {
          record % action  // 1
              % status;    // 2
          if (order.price) {
            record % *order.price;  // 3
          } else {
            record % "null";  // 3
          }
          if (filledQty) {
            record % m_self.GetLastTradePrice();  // 4
          } else {
            record % "null";  // 4
          }
          record % order.qty                            // 5
              % filledQty                               // 6
              % (order.qty - filledQty)                 // 7
              % order.transactionContext->GetOrderId()  // 8
              % m_open.startPrice                       // 9
              % m_planedQty                             // 10
              % m_self.GetActiveQty()                   // 11
              % m_self.GetSide()                        // 12
              % *m_security                             // 13
              % m_operation->GetId()                    // 14
              % m_subOperationId;                       // 15
        });
  }

 public:
  void RestoreOpenState(
      const pt::ptime &openStartTime,
      const pt::ptime &openTime,
      const Price &openPrice,
      const boost::shared_ptr<const OrderTransactionContext> &openingContext) {
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

    bool isRegistered = false;
    if (!m_isRegistered) {
      m_strategy.Register(m_self);
      isRegistered = true;
    }

    if (!m_security->GetSymbol().IsExplicit()) {
      m_expiration = m_security->GetExpiration();
    }

    m_open.orders.emplace_back(std::move(openStartTime), boost::none,
                               m_planedQty);
    try {
      auto &order = m_open.orders.back();
      order.transactionContext = openingContext;
      order.status = ORDER_STATUS_STEP_CLOSED;

      m_open.time = openTime;
      m_open.AddTrade(order.qty, openPrice);

      if (order.transactionContext) {
        m_defaultOrderParams.position = &*order.transactionContext;
      }

      if (isRegistered) {
        Assert(!m_isRegistered);
        m_isRegistered = true;
      }

    } catch (...) {
      if (isRegistered) {
        Assert(!m_isRegistered);
        m_strategy.Unregister(m_self);
      }
      m_open.orders.pop_back();
      throw;
    }

    ReportOpeningStateRestoration();
  }

  template <typename OpenImpl>
  const OrderTransactionContext &Open(const OpenImpl &openImpl,
                                      const OrderParams &orderParams,
                                      boost::optional<Price> &&price) {
    const auto now = m_strategy.GetContext().GetCurrentTime();

    if (!m_security->IsOnline()) {
      throw Exception("Security is not online");
    } else if (!m_security->IsTradingSessionOpened()) {
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

    bool isRegistered = false;
    if (!m_isRegistered) {
      m_strategy.Register(m_self);
      isRegistered = true;
    }

    m_open.orders.emplace_back(std::move(now), std::move(price), qty);
    ReportOpeningStart("start", boost::none);
    auto &order = m_open.orders.back();

    try {
      auto handler =
          boost::make_unique<OpenStatusHandler>(m_self.shared_from_this());
      if (!orderParams.expiration && !m_security->GetSymbol().IsExplicit()) {
        m_expiration = m_security->GetExpiration();
        OrderParams additionalOrderParams(orderParams);
        additionalOrderParams.expiration = &*m_expiration;
        order.transactionContext =
            openImpl(qty, additionalOrderParams, std::move(handler));
      } else {
        order.transactionContext =
            openImpl(qty, orderParams, std::move(handler));
      }
      Assert(order.transactionContext);

      ReportOpeningStart("sent", order.transactionContext->GetOrderId());

      if (isRegistered) {
        Assert(!m_isRegistered);
        m_isRegistered = true;
      }

    } catch (...) {
      if (isRegistered) {
        Assert(!m_isRegistered);
        m_strategy.Unregister(m_self);
      }
      m_open.orders.pop_back();
      throw;
    }

    return *order.transactionContext;
  }

  template <typename CloseImpl>
  const OrderTransactionContext &Close(const CloseImpl &closeImpl,
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
      auto handler =
          boost::make_unique<CloseStatusHandler>(m_self.shared_from_this());
      if (m_expiration && !orderParams.expiration) {
        OrderParams additionalOrderParams(orderParams);
        additionalOrderParams.expiration = &*m_expiration;
        order.transactionContext =
            closeImpl(order.qty, additionalOrderParams, std::move(handler));
      } else {
        order.transactionContext =
            closeImpl(order.qty, orderParams, std::move(handler));
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
    AssertGe(ORDER_STATUS_STEP_OPENED, order.status);
    Assert(!order.isCanceled);
    m_strategy.GetTradingLog().Write(
        "position\tcancel_all\t%1%_order\t%2%\t%3%\t%4%"
        "\toperation=%5%/%6%\torder=%7%",
        [this, &order, &direction](TradingRecord &record) {
          record % direction                                         // 1
              % m_security->GetSymbol().GetSymbol().c_str()          // 2
              % m_self.GetTradingSystem().GetInstanceName().c_str()  // 3
              % m_tradingSystem->GetMode()                           // 4
              % m_operation->GetId()                                 // 5
              % m_subOperationId                                     // 6
              % order.transactionContext->GetOrderId();              // 7
        });
    m_tradingSystem->CancelOrder(order.transactionContext->GetOrderId());
    order.isCanceled = true;
    return true;
  }
};

//////////////////////////////////////////////////////////////////////////

namespace {

class DummyOperation : public Operation {
 public:
  virtual ~DummyOperation() override = default;

 public:
  virtual const OrderPolicy &GetOpenOrderPolicy(
      const Position &) const override {
    throw LogicError("Position instance does not use operation context");
  }
  virtual const OrderPolicy &GetCloseOrderPolicy(
      const Position &) const override {
    throw LogicError("Position instance does not use operation context");
  }
  virtual void Setup(Position &, PositionController &) const override {
    throw LogicError("Position instance does not use operation context");
  }
  virtual bool IsLong(const Security &) const override {
    throw LogicError("Position instance does not use operation context");
  }
  virtual Qty GetPlannedQty() const override {
    throw LogicError("Position instance does not use operation context");
  }
};
}
Position::Position(Strategy &strategy,
                   TradingSystem &tradingSystem,
                   Security &security,
                   const Currency &currency,
                   const Qty &qty,
                   const Price &startPrice,
                   const TimeMeasurement::Milestones &timeMeasurement)
    : m_pimpl(
          std::make_unique<Implementation>(*this,
                                           boost::make_shared<DummyOperation>(),
                                           tradingSystem,
                                           strategy,
                                           0,
                                           security,
                                           currency,
                                           qty,
                                           startPrice,
                                           timeMeasurement)) {
  Assert(!strategy.IsBlocked());
  ++objectsCounter;
}

Position::Position(const boost::shared_ptr<Operation> &operation,
                   int64_t subOperationId,
                   Strategy &strategy,
                   Security &security,
                   const Currency &currency,
                   const Qty &qty,
                   const Price &startPrice,
                   const TimeMeasurement::Milestones &timeMeasurement)
    : m_pimpl(std::make_unique<Implementation>(
          *this,
          operation,
          operation->GetTradingSystem(strategy, security),
          strategy,
          subOperationId,
          security,
          currency,
          qty,
          startPrice,
          timeMeasurement)) {
  Assert(!strategy.IsBlocked());
  ++objectsCounter;
}

Position::~Position() {
  AssertLt(0, objectsCounter);
  --objectsCounter;
}

int64_t Position::GetSubOperationId() const {
  return m_pimpl->m_subOperationId;
}

bool Position::IsLong() const {
  static_assert(numberOfPositionSides == 2, "List changed.");
  Assert(GetSide() == POSITION_SIDE_LONG || GetSide() == POSITION_SIDE_SHORT);
  return GetSide() == POSITION_SIDE_LONG;
}

const boost::shared_ptr<Operation> &Position::GetOperation() {
  return m_pimpl->m_operation;
}
const boost::shared_ptr<const Operation> &Position::GetOperation() const {
  const boost::shared_ptr<const Operation> &result =
      reinterpret_cast<const boost::shared_ptr<const Operation> &>(
          m_pimpl->m_operation);
  return result;
}

const ContractExpiration &Position::GetExpiration() const {
  if (!m_pimpl->m_expiration) {
    Assert(m_pimpl->m_expiration);
    boost::format error("Position %1% %2%/%3% does not have expiration");
    error % GetSecurity()          // 1
        % GetOperation()->GetId()  // 2
        % GetSubOperationId();     // 3
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
Security &Position::GetSecurity() noexcept { return *m_pimpl->m_security; }

const TradingSystem &Position::GetTradingSystem() const {
  return const_cast<Position *>(this)->GetTradingSystem();
}

TradingSystem &Position::GetTradingSystem() {
  return *m_pimpl->m_tradingSystem;
}

void Position::ReplaceTradingSystem(Security &security,
                                    TradingSystem &tradingSystem) {
  if (HasActiveOrders()) {
    throw Exception(
        "Filled to replace trading system as position has active orders");
  }
  if (m_pimpl->m_security == &security &&
      m_pimpl->m_tradingSystem == &tradingSystem) {
    return;
  }
  GetStrategy().GetTradingLog().Write(
      "{'position': {'move': {'security': {'prev': {'security': '%1%', 'bid': "
      "%2$.8f, 'ask':%3$.8f}, 'new': {'security': '%4%', 'bid': %5$.8f, 'ask': "
      "%6$.8f}}, 'tradingSystem': {'prev': {'name': '%7%', 'mode': '%8%'}, "
      "'new': 'name': '%9%', 'mode': '%10%'}}}, 'startPrice': %11$.8f, 'qty': "
      "%12$.8f, 'type': '%13%', 'security': '%14%', 'operation': '%15%/%16%'}}",
      [&](TradingRecord &record) {
        record % *m_pimpl->m_security                              // 1
            % m_pimpl->m_security->GetBidPriceValue()              // 2
            % m_pimpl->m_security->GetAskPriceValue()              // 3
            % security                                             // 4
            % security.GetBidPriceValue()                          // 5
            % security.GetAskPriceValue()                          // 6
            % m_pimpl->m_tradingSystem->GetInstanceName().c_str()  // 7
            % m_pimpl->m_tradingSystem->GetMode()                  // 8
            % tradingSystem.GetInstanceName().c_str()              // 9
            % tradingSystem.GetMode()                              // 10
            % GetOpenStartPrice()                                  // 11
            % GetPlanedQty()                                       // 12
            % GetSide()                                            // 13
            % GetSecurity()                                        // 14
            % m_pimpl->m_operation->GetId()                        // 15
            % m_pimpl->m_subOperationId;                           // 16
      });
  m_pimpl->m_security = &security;
  m_pimpl->m_tradingSystem = &tradingSystem;
}

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
  if (m_pimpl->m_operation->OnCloseReasonChange(*this, newCloseReason)) {
    m_pimpl->m_closeReason = newCloseReason;
  }
}

void Position::ResetCloseReason(const CloseReason &newReason) {
  if (m_pimpl->m_operation->OnCloseReasonChange(*this, newReason)) {
    m_pimpl->m_closeReason = newReason;
  }
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
  Assert(!IsCompleted());
  if (IsCompleted()) {
    // Should not be added to the "delayed list" twice.
    GetStrategy().GetLog().Error(
        "Failed to mark position %1%/%2% as \"completed\": position already "
        "completed (%3%).",
        GetOperation()->GetId(),                                // 1
        GetSubOperationId(),                                    // 2
        m_pimpl->m_isMarketAsCompleted ? "forced" : "native");  // 3
    return;
  }
  ReportAboutGeneralAction(*this, "forcing", "completed");
  AssertEq(m_pimpl->m_close.time, pt::not_a_date_time);
  m_pimpl->m_close.time = GetSecurity().GetContext().GetCurrentTime();
  m_pimpl->m_isMarketAsCompleted = true;
  m_pimpl->m_strategy.OnPositionMarkedAsCompleted(*this);
}

bool Position::IsError() const noexcept { return m_pimpl->m_isError; }

bool Position::IsRejected() const {
  const auto &orders = !m_pimpl->m_close.orders.empty()
                           ? m_pimpl->m_close.orders
                           : m_pimpl->m_open.orders;
  if (orders.empty()) {
    return false;
  }
  return orders.back().isRejected;
}

bool Position::IsCancelling() const {
  return m_pimpl->m_open.IsCanceling() || m_pimpl->m_close.IsCanceling();
}

bool Position::HasActiveOrders() const noexcept {
  return HasActiveCloseOrders() || HasActiveOpenOrders();
}
bool Position::HasOpenedOrders() const noexcept {
  return HasOpenedCloseOrders() || HasOpenedOpenOrders();
}
bool Position::HasActiveOpenOrders() const noexcept {
  return m_pimpl->m_open.HasActiveOrders();
}
bool Position::HasOpenedOpenOrders() const noexcept {
  return m_pimpl->m_open.HasOpenedOrders();
}
bool Position::HasActiveCloseOrders() const noexcept {
  return m_pimpl->m_close.HasActiveOrders();
}
bool Position::HasOpenedCloseOrders() const noexcept {
  return m_pimpl->m_close.HasOpenedOrders();
}

const pt::ptime &Position::GetOpenTime() const { return m_pimpl->m_open.time; }

namespace {

template <typename Source>
const boost::shared_ptr<const OrderTransactionContext> &GetOrderContext(
    const Source &source, size_t index) {
  AssertLt(index, source.orders.size());
  if (index >= source.orders.size()) {
    throw Exception("Position has no order to have order context");
  }
  const auto &result = source.orders[index].transactionContext;
  if (!result) {
    throw Exception("Position order has no context");
  }
  return result;
}
}
const boost::shared_ptr<const OrderTransactionContext>
    &Position::GetOpeningContext(size_t index) const {
  return GetOrderContext(m_pimpl->m_open, index);
}
const boost::shared_ptr<const OrderTransactionContext>
    &Position::GetClosingContext(size_t index) const {
  return GetOrderContext(m_pimpl->m_close, index);
}

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
void Position::SetClosedQty(const Qty &newValue) {
  AssertGe(GetOpenedQty(), newValue);
  m_pimpl->m_close.qty = newValue;
  ReportAboutGeneralAction(*this, "forcing", "closedQty");
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
size_t Position::GetNumberOfCloseOrders() const {
  return m_pimpl->m_close.orders.size();
}

size_t Position::GetNumberOfTrades() const {
  return GetNumberOfOpenTrades() + GetNumberOfCloseTrades();
}
size_t Position::GetNumberOfOpenTrades() const {
  return m_pimpl->m_open.numberOfTrades;
}
size_t Position::GetNumberOfCloseTrades() const {
  return m_pimpl->m_close.numberOfTrades;
}

Position::StateUpdateConnection Position::Subscribe(
    const StateUpdateSlot &slot) const {
  return StateUpdateConnection(m_pimpl->m_stateUpdateSignal.connect(slot));
}

void Position::ScheduleUpdateEvent(const pt::time_duration &delay) {
  GetStrategy().GetContext().GetTimer().Schedule(
      delay, [this]() { m_pimpl->SignalUpdate(); }, m_pimpl->m_timerScope);
}

void Position::AddAlgo(std::unique_ptr<Algo> &&algo) {
  Assert(algo);
  m_pimpl->m_algos.emplace_back(std::move(algo));
  m_pimpl->m_algos.back()->Report("add");
}

void Position::RemoveAlgos() {
  for (const auto &algo : m_pimpl->m_algos) {
    algo->Report("remove");
  }
  m_pimpl->m_algos.clear();
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

Price Position::GetOpenAvgPrice() const {
  if (!m_pimpl->m_open.qty) {
    throw Exception("Position has no open price");
  }
  return RoundByPrecision(m_pimpl->m_open.volume / m_pimpl->m_open.qty,
                          GetSecurity().GetPricePrecisionPower());
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
  return RoundByPrecision(m_pimpl->m_close.volume / m_pimpl->m_close.qty,
                          GetSecurity().GetPricePrecisionPower());
}

const boost::optional<Price> &Position::GetActiveCloseOrderPrice() const {
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

void Position::RestoreOpenState(
    const pt::ptime &openStartTime,
    const pt::ptime &openTime,
    const Price &openPrice,
    const boost::shared_ptr<const OrderTransactionContext> &openingContext) {
  m_pimpl->RestoreOpenState(openStartTime, openTime, openPrice, openingContext);
}

void Position::AddVirtualTrade(const Qty &qty, const Price &price) {
  if (m_pimpl->m_close.CheckAndAddVirtualTrade(qty, price)) {
    m_pimpl->ReportAction("forcing", "trade", m_pimpl->m_close.orders.back(),
                          GetClosedQty());
  } else if (m_pimpl->m_open.CheckAndAddVirtualTrade(qty, price)) {
    m_pimpl->ReportAction("forcing", "trade", m_pimpl->m_open.orders.back(),
                          GetOpenedQty());
  } else {
    throw Exception("There are no active orders to add virtual trade");
  }
}

const OrderTransactionContext &Position::OpenAtMarketPrice() {
  return OpenAtMarketPrice(m_pimpl->m_defaultOrderParams);
}

const OrderTransactionContext &Position::OpenAtMarketPrice(
    const OrderParams &params) {
  return m_pimpl->Open(
      [this](const Qty &qty, const OrderParams &params,
             std::unique_ptr<OrderStatusHandler> &&handler) {
        return DoOpenAtMarketPrice(qty, params, std::move(handler));
      },
      params, boost::none);
}

const OrderTransactionContext &Position::Open(const Price &price) {
  return Open(price, m_pimpl->m_defaultOrderParams);
}

const OrderTransactionContext &Position::Open(const Price &price,
                                              const OrderParams &params) {
  return m_pimpl->Open(
      [this, &price](const Qty &qty, const OrderParams &params,
                     std::unique_ptr<OrderStatusHandler> &&handler) {
        return DoOpen(qty, price, params, std::move(handler));
      },
      params, price);
}

const OrderTransactionContext &Position::OpenImmediatelyOrCancel(
    const Price &price) {
  return OpenImmediatelyOrCancel(price, m_pimpl->m_defaultOrderParams);
}

const OrderTransactionContext &Position::OpenImmediatelyOrCancel(
    const Price &price, const OrderParams &params) {
  return m_pimpl->Open(
      [this, price](const Qty &qty, const OrderParams &params,
                    std::unique_ptr<OrderStatusHandler> &&handler) {
        return DoOpenImmediatelyOrCancel(qty, price, params,
                                         std::move(handler));
      },
      params, price);
}

const OrderTransactionContext &Position::CloseAtMarketPrice() {
  return CloseAtMarketPrice(m_pimpl->m_defaultOrderParams);
}

const OrderTransactionContext &Position::CloseAtMarketPrice(
    const OrderParams &params) {
  return m_pimpl->Close(
      [this](const Qty &qty, const OrderParams &params,
             std::unique_ptr<OrderStatusHandler> &&handler) {
        return DoCloseAtMarketPrice(qty, params, std::move(handler));
      },
      boost::none, GetActiveQty(), params);
}

const OrderTransactionContext &Position::Close(const Price &price) {
  return Close(price, GetActiveQty(), m_pimpl->m_defaultOrderParams);
}

const OrderTransactionContext &Position::Close(const Price &price,
                                               const Qty &maxQty) {
  return Close(price, maxQty, m_pimpl->m_defaultOrderParams);
}

const OrderTransactionContext &Position::Close(const Price &price,
                                               const OrderParams &params) {
  return Close(price, GetActiveQty(), params);
}

const OrderTransactionContext &Position::Close(const Price &price,
                                               const Qty &maxQty,
                                               const OrderParams &params) {
  return m_pimpl->Close(
      [this, &price](const Qty &qty, const OrderParams &params,
                     std::unique_ptr<OrderStatusHandler> &&handler) {
        return DoClose(qty, price, params, std::move(handler));
      },
      price, maxQty, params);
}

const OrderTransactionContext &Position::CloseImmediatelyOrCancel(
    const Price &price) {
  return CloseImmediatelyOrCancel(price, m_pimpl->m_defaultOrderParams);
}

const OrderTransactionContext &Position::CloseImmediatelyOrCancel(
    const Price &price, const OrderParams &params) {
  return m_pimpl->Close(
      [this, price](const Qty &qty, const OrderParams &params,
                    std::unique_ptr<OrderStatusHandler> &&handler) {
        return DoCloseImmediatelyOrCancel(qty, price, params,
                                          std::move(handler));
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

//////////////////////////////////////////////////////////////////////////

LongPosition::LongPosition(Strategy &strategy,
                           TradingSystem &tradingSystem,
                           Security &security,
                           const Currency &currency,
                           const Qty &qty,
                           const Price &startPrice,
                           const TimeMeasurement::Milestones &timeMeasurement)
    : Position(strategy,
               tradingSystem,
               security,
               currency,
               qty,
               startPrice,
               timeMeasurement) {
  GetStrategy().GetTradingLog().Write(
      "position\tnew\tlong\t%1%\t%2%.%3%\tprice=%4$.8f\t%5%\tqty=%6$.8f"
      "\toperation=%7%/%8%\tparent=%9%",
      [this](TradingRecord &record) {
        record % GetSecurity().GetSymbol().GetSymbol().c_str()  // 1
            % GetTradingSystem().GetInstanceName().c_str()      // 2
            % GetTradingSystem().GetMode()                      // 3
            % GetOpenStartPrice()                               // 4
            % GetCurrency()                                     // 5
            % GetPlanedQty()                                    // 6
            % GetOperation()->GetId()                           // 7
            % GetSubOperationId();                              // 8
        {
          const auto &parent = GetOperation()->GetParent();
          if (parent) {
            record % parent->GetId();  // 9
          } else {
            record % '-';  // 9
          }
        }
      });
}

LongPosition::LongPosition(const boost::shared_ptr<Operation> &operation,
                           int64_t subOperationId,
                           Strategy &strategy,
                           Security &security,
                           const Currency &currency,
                           const Qty &qty,
                           const Price &startPrice,
                           const TimeMeasurement::Milestones &timeMeasurement)
    : Position(operation,
               subOperationId,
               strategy,
               security,
               currency,
               qty,
               startPrice,
               timeMeasurement) {
  GetStrategy().GetTradingLog().Write(
      "position\tnew\tlong\t%1%\t%2%.%3%\tprice=%4$.8f\t%5%\tqty=%6$.8f"
      "\toperation=%7%/%8%\tparent=%9%",
      [this](TradingRecord &record) {
        record % GetSecurity().GetSymbol().GetSymbol().c_str()  // 1
            % GetTradingSystem().GetInstanceName().c_str()      // 2
            % GetTradingSystem().GetMode()                      // 3
            % GetOpenStartPrice()                               // 4
            % GetCurrency()                                     // 5
            % GetPlanedQty()                                    // 6
            % GetOperation()->GetId()                           // 7
            % GetSubOperationId();                              // 8
        {
          const auto &parent = GetOperation()->GetParent();
          if (parent) {
            record % parent->GetId();  // 9
          } else {
            record % '-';  // 9
          }
        }
      });
}

LongPosition::~LongPosition() { ReportAboutDeletion(*this); }

PositionSide LongPosition::GetSide() const { return POSITION_SIDE_LONG; }

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

boost::shared_ptr<const OrderTransactionContext>
LongPosition::DoOpenAtMarketPrice(
    const Qty &qty,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, boost::none, params,
      std::move(handler), GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY,
      TIME_IN_FORCE_GTC, GetTimeMeasurement());
}

boost::shared_ptr<const OrderTransactionContext> LongPosition::DoOpen(
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params, std::move(handler),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY, TIME_IN_FORCE_GTC,
      GetTimeMeasurement());
}

boost::shared_ptr<const OrderTransactionContext>
LongPosition::DoOpenImmediatelyOrCancel(
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params, std::move(handler),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY, TIME_IN_FORCE_IOC,
      GetTimeMeasurement());
}

boost::shared_ptr<const OrderTransactionContext>
LongPosition::DoCloseAtMarketPrice(
    const Qty &qty,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, boost::none, params,
      std::move(handler), GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL,
      TIME_IN_FORCE_GTC, GetTimeMeasurement());
}

boost::shared_ptr<const OrderTransactionContext> LongPosition::DoClose(
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params, std::move(handler),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL, TIME_IN_FORCE_GTC,

      GetTimeMeasurement());
}

boost::shared_ptr<const OrderTransactionContext>
LongPosition::DoCloseImmediatelyOrCancel(
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(IsOpened());
  Assert(!IsClosed());
  AssertLt(0, qty);
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params, std::move(handler),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL, TIME_IN_FORCE_IOC,
      GetTimeMeasurement());
}

//////////////////////////////////////////////////////////////////////////

ShortPosition::ShortPosition(Strategy &strategy,
                             TradingSystem &tradingSystem,
                             Security &security,
                             const Currency &currency,
                             const Qty &qty,
                             const Price &startPrice,
                             const TimeMeasurement::Milestones &timeMeasurement)
    : Position(strategy,
               tradingSystem,
               security,
               currency,
               qty,
               startPrice,
               timeMeasurement) {
  GetStrategy().GetTradingLog().Write(
      "position\tnew\tshort\t%1%\t%2%.%3%\tprice=%4$.8f\t%5%\tqty=%6$.8f"
      "\toperation=%7%/%8%\tparent=%9%",
      [this](TradingRecord &record) {
        record % GetSecurity().GetSymbol().GetSymbol().c_str()  // 1
            % GetTradingSystem().GetInstanceName().c_str()      // 2
            % GetTradingSystem().GetMode()                      // 3
            % GetOpenStartPrice()                               // 4
            % GetCurrency()                                     // 5
            % GetPlanedQty()                                    // 6
            % GetOperation()->GetId()                           // 7
            % GetSubOperationId();                              // 8
        {
          const auto &parent = GetOperation()->GetParent();
          if (parent) {
            record % parent->GetId();  // 9
          } else {
            record % '-';  // 9
          }
        }
      });
}

ShortPosition::ShortPosition(const boost::shared_ptr<Operation> &operation,
                             int64_t subOperationId,
                             Strategy &strategy,
                             Security &security,
                             const Currency &currency,
                             const Qty &qty,
                             const Price &startPrice,
                             const TimeMeasurement::Milestones &timeMeasurement)
    : Position(operation,
               subOperationId,
               strategy,
               security,
               currency,
               qty,
               startPrice,
               timeMeasurement) {
  GetStrategy().GetTradingLog().Write(
      "position\tnew\tshort\t%1%\t%2%.%3%\tprice=%4$.8f\t%5%\tqty=%6$.8f"
      "\toperation=%7%/%8%\tparent=%9%",
      [this](TradingRecord &record) {
        record % GetSecurity().GetSymbol().GetSymbol().c_str()  // 1
            % GetTradingSystem().GetInstanceName().c_str()      // 2
            % GetTradingSystem().GetMode()                      // 3
            % GetOpenStartPrice()                               // 4
            % GetCurrency()                                     // 5
            % GetPlanedQty()                                    // 6
            % GetOperation()->GetId()                           // 7
            % GetSubOperationId();                              // 8
        {
          const auto &parent = GetOperation()->GetParent();
          if (parent) {
            record % parent->GetId();  // 9
          } else {
            record % '-';  // 9
          }
        }
      });
}

ShortPosition::~ShortPosition() { ReportAboutDeletion(*this); }

PositionSide ShortPosition::GetSide() const { return POSITION_SIDE_SHORT; }

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

boost::shared_ptr<const OrderTransactionContext>
ShortPosition::DoOpenAtMarketPrice(
    const Qty &qty,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, boost::none, params,
      std::move(handler), GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL,
      TIME_IN_FORCE_GTC, GetTimeMeasurement());
}

boost::shared_ptr<const OrderTransactionContext> ShortPosition::DoOpen(
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params, std::move(handler),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL, TIME_IN_FORCE_GTC,
      GetTimeMeasurement());
}

boost::shared_ptr<const OrderTransactionContext>
ShortPosition::DoOpenImmediatelyOrCancel(
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(!IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params, std::move(handler),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_SELL, TIME_IN_FORCE_IOC,
      GetTimeMeasurement());
}

boost::shared_ptr<const OrderTransactionContext>
ShortPosition::DoCloseAtMarketPrice(
    const Qty &qty,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, boost::none, params,
      std::move(handler), GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY,
      TIME_IN_FORCE_GTC, GetTimeMeasurement());
}

boost::shared_ptr<const OrderTransactionContext> ShortPosition::DoClose(
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params, std::move(handler),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY, TIME_IN_FORCE_GTC,
      GetTimeMeasurement());
}

boost::shared_ptr<const OrderTransactionContext>
ShortPosition::DoCloseImmediatelyOrCancel(
    const Qty &qty,
    const Price &price,
    const OrderParams &params,
    std::unique_ptr<OrderStatusHandler> &&handler) {
  Assert(IsOpened());
  Assert(!IsClosed());
  return GetTradingSystem().SendOrder(
      GetSecurity(), GetCurrency(), qty, price, params, std::move(handler),
      GetStrategy().GetRiskControlScope(), ORDER_SIDE_BUY, TIME_IN_FORCE_IOC,
      GetTimeMeasurement());
}

//////////////////////////////////////////////////////////////////////////
