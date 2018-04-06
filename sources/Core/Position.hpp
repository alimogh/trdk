/**************************************************************************
 *   Created: 2012/07/09 17:32:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "TradingLib/Fwd.hpp"
#include "Api.h"
#include "Security.hpp"

namespace trdk {

class LongPosition;
class ShortPosition;

//////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API Position : boost::noncopyable,
                               public boost::enable_shared_from_this<Position> {
 public:
  typedef void(StateUpdateSlotSignature)();
  typedef boost::function<StateUpdateSlotSignature> StateUpdateSlot;
  typedef boost::signals2::connection StateUpdateConnection;

  class TRDK_CORE_API Exception : public Lib::Exception {
   public:
    explicit Exception(const char *what) noexcept;
  };

  class TRDK_CORE_API AlreadyStartedError : public Exception {
   public:
    AlreadyStartedError() noexcept;
  };
  class TRDK_CORE_API NotOpenedError : public Exception {
   public:
    NotOpenedError() noexcept;
  };

  class TRDK_CORE_API AlreadyClosedError : public Exception {
   public:
    AlreadyClosedError() noexcept;
  };

  explicit Position(const boost::shared_ptr<Operation> &,
                    int64_t subOperationId,
                    Security &,
                    const Lib::Currency &,
                    const Qty &,
                    const Price &startPrice,
                    const Lib::TimeMeasurement::Milestones &);
  virtual ~Position();

  int64_t GetSubOperationId() const;

  //! Adds algorithm to the position.
  /** Will try to execute algorithm at each price update, but only if position
   * is not in the "canceling state".
   * @sa IsCancelling
   * @sa IsCompleted
   */
  void AddAlgo(std::unique_ptr<TradingLib::Algo> &&);
  //! Removes all algorithms.
  void RemoveAlgos();

  const boost::shared_ptr<Operation> &GetOperation();
  const boost::shared_ptr<const Operation> &GetOperation() const;

  template <typename Operation>
  Operation &GetTypedOperation() {
    return *boost::polymorphic_downcast<Operation *>(&*GetOperation());
  }
  template <typename Operation>
  const Operation &GetTypedOperation() const {
    return *boost::polymorphic_downcast<const Operation *>(&*GetOperation());
  }

  bool IsLong() const;
  virtual PositionSide GetSide() const = 0;
  virtual OrderSide GetOpenOrderSide() const = 0;
  virtual OrderSide GetCloseOrderSide() const = 0;

  const Lib::ContractExpiration &GetExpiration() const;

  const TradingSystem &GetTradingSystem() const;
  TradingSystem &GetTradingSystem();
  void ReplaceTradingSystem(Security &, TradingSystem &);

  const Strategy &GetStrategy() const noexcept;
  Strategy &GetStrategy() noexcept;

  const Security &GetSecurity() const noexcept;
  Security &GetSecurity() noexcept;

  const Lib::Currency &GetCurrency() const;

  const Lib::TimeMeasurement::Milestones &GetTimeMeasurement();

  const CloseReason &GetCloseReason() const noexcept;

  //! Sets position closing reason only if reason was not set before.
  /** Position may be completed immediately after reason set if conditions are
   * met.
   * @sa ResetCloseReason
   * @sa IsCompleted
   */
  void SetCloseReason(const CloseReason &);
  //! Sets or changes position closing reason.
  /** Position may be completed immediately after reason set if conditions are
   * met.
   * @sa SetCloseReason
   * @sa IsCompleted
   */
  void ResetCloseReason(const CloseReason & = CLOSE_REASON_NONE);

  //! Has opened quantity equals or more than planned quantity.
  /**
   * @sa GetPlanedQty
   * @sa GetActiveQty
   * @sa IsOpened
   */
  bool IsFullyOpened() const;
  //! Has opened qty and doesn't have active open-orders.
  /**
   * @sa IsClosed
   * @sa IsFullyOpened
   */
  bool IsOpened() const noexcept;
  //! Closed.
  /** First was opened, then closed, doesn't have active quantity and
   * active orders.
   * @sa IsOpened
   */
  bool IsClosed() const noexcept;

  //! Has any opened qty or has close reason, and now doesn't have any orders
  //! and active qty or market as completed.
  /**
   * @sa MarkAsCompleted
   * @sa SetCloseReason
   * @sa ResetCloseReason
   */
  bool IsCompleted() const noexcept;
  //! Marks position as completed without real closing.
  /** Call not thread-safe! Must be called only from event-methods, or if
   * strategy locked by GetMutex().
   * @sa IsCompleted
   */
  void MarkAsCompleted();

  //! Open operation started, but error occurred at opening or closing.
  bool IsError() const noexcept;

  //! Last order was rejected.
  /** @sa ResetRejected
   */
  bool IsRejected() const;

  bool HasActiveOrders() const noexcept;
  bool HasOpenedOrders() const noexcept;
  bool HasActiveOpenOrders() const noexcept;
  bool HasOpenedOpenOrders() const noexcept;
  bool HasActiveCloseOrders() const noexcept;
  bool HasOpenedCloseOrders() const noexcept;

  const Qty &GetPlanedQty() const;
  const Price &GetOpenStartPrice() const;
  //! Returns time of first order.
  /** Throws an exception if there is no open-order at this moment.
   * @sa GetCloseStartTime()
   * @sa GetActiveOrderTime()
   */
  const boost::posix_time::ptime &GetOpenStartTime() const;

  const Qty &GetOpenedQty() const noexcept;
  Price GetOpenAvgPrice() const;
  //! Returns price of active open-order.
  /** Throws an exception if there is no active open-order at this moment.
   * @sa GetActiveOrderPrice
   * @sa GetActiveCloseOrderPrice
   * @sa GetActiveOpenOrderTime
   * @return Active order price or boost::none if the order is an order by
   *         market price.
   */
  const boost::optional<Price> &GetActiveOpenOrderPrice() const;
  //! Returns time of active open-order.
  /** Throws an exception if there is no active open-order at this moment.
   * @sa GetActiveOrderTime
   * @sa GetOpenStartTime
   * @sa GetActiveCloseOrderTime
   */
  const boost::posix_time::ptime &GetActiveOpenOrderTime() const;
  //! Returns price of last open-trade.
  /** Throws an exception if there are no open-trades yet.
   * @sa GetLastTradePrice
   * @sa GetLastCloseTradePrice
   */
  const Price &GetLastOpenTradePrice() const;
  Volume GetOpenedVolume() const;
  //! Time of first open-trade.
  const boost::posix_time::ptime &GetOpenTime() const;

  //! Returns position opening context if exists or throws an exception if
  //! doesn't have it.
  /** @sa GetNumberOfOpenOrders
   */
  const boost::shared_ptr<const OrderTransactionContext> &GetOpeningContext(
      size_t index) const;
  //! Returns position closing context if exists or throws an exception if
  //! doesn't have it.
  /** @sa GetNumberOfCloseOrders
   */
  const boost::shared_ptr<const OrderTransactionContext> &GetClosingContext(
      size_t index) const;

  Qty GetActiveQty() const noexcept;
  Volume GetActiveVolume() const;

  void SetCloseStartPrice(const Price &);
  const Price &GetCloseStartPrice() const;
  //! Returns time of first close-order.
  /** Throws an exception if there is no close-order at this moment.
   * @sa GetOpenStartTime()
   * @sa GetActiveCloseTime()
   */
  const boost::posix_time::ptime &GetCloseStartTime() const;
  Price GetCloseAvgPrice() const;
  //! Returns price of active close-order.
  /** Throws an exception if there is no active close-order at this moment.
   * @sa GetActiveOrderPrice
   * @sa GetActiveOpenOrderPrice
   * @sa GetActiveCloseOrderTime
   * @return Active order price or boost::none if the order is an order by
   *         market price.
   */
  const boost::optional<Price> &GetActiveCloseOrderPrice() const;
  //! Returns time of active close-order.
  /** Throws an exception if there is no active close-order at this
   * moment.
   * @sa GetActiveOrderTime
   * @sa GetActiveOpenOrderTime
   * @sa GetActiveCloseTime
   */
  const boost::posix_time::ptime &GetActiveCloseOrderTime() const;
  //! Returns price of last close-trade.
  /** Throws an exception if there are no close-trades yet.
   * @sa GetLastTradePrice
   * @sa GetLastOpenTradePrice
   */
  const Price &GetLastCloseTradePrice() const;
  const Qty &GetClosedQty() const noexcept;
  void SetClosedQty(const Qty &);
  Volume GetClosedVolume() const;
  //! Time of last trade.
  const boost::posix_time::ptime &GetCloseTime() const;

  //! Returns price of active order.
  /** Throws an exception if there is no active order at this moment.
   * @sa GetActiveOpenOrderPrice
   * @sa GetActiveOpenClosePrice
   * @return Active order price or boost::none if the order is an order by
   *         market price.
   */
  const boost::optional<Price> &GetActiveOrderPrice() const;
  //! Returns time of active order.
  /** Throws an exception if there is no active order at this moment.
   * @sa GetActiveOpenOrderTime
   * @sa GetActiveOpenCloseTime
   */
  const boost::posix_time::ptime &GetActiveOrderTime() const;
  //! Returns price of last trade.
  /** Throws an exception if there are no trades yet.
   * @sa GetLastOpenTradePrice
   * @sa GetLastCloseTradePrice
   */
  const Price &GetLastTradePrice() const;

  virtual Volume GetRealizedPnl() const = 0;
  Volume GetRealizedPnlVolume() const;
  //! Returns realized PnL ratio.
  /** @return If the value is 1.0 - no profit and no loss,
   *          if less then 1.0 - loss, if greater then 1.0 - profit.
   */
  virtual Lib::Double GetRealizedPnlRatio() const = 0;
  //! Returns percentage of profit.
  /** @return If the value is zero - no profit and no loss,
   *          if less then zero - loss, if greater then zero - profit.
   */
  Lib::Double GetRealizedPnlPercentage() const;
  virtual Volume GetUnrealizedPnl() const = 0;
  //! Realized PnL + unrealized PnL.
  Volume GetPlannedPnl() const;

  virtual Price GetMarketOpenPrice() const = 0;
  virtual Price GetMarketClosePrice() const = 0;
  virtual Price GetMarketOpenOppositePrice() const = 0;
  virtual Price GetMarketCloseOppositePrice() const = 0;

  size_t GetNumberOfOrders() const;
  size_t GetNumberOfOpenOrders() const;
  size_t GetNumberOfCloseOrders() const;

  size_t GetNumberOfTrades() const;
  size_t GetNumberOfOpenTrades() const;
  size_t GetNumberOfCloseTrades() const;

  //! Restores position in open-state.
  /** Sets position in open state at current strategy, doesn't make any trading
   *  actions.
   *  @param[in] openStartTime  Position opening start time.
   *  @param[in] openTime       Position opening time.
   *  @param[in] openPrice      Position opening price.
   *  @param[in] openingContext Position opening context, if exists.
   */
  void RestoreOpenState(const boost::posix_time::ptime &openStartTime,
                        const boost::posix_time::ptime &openTime,
                        const Price &openPrice,
                        const boost::shared_ptr<const OrderTransactionContext>
                            &openingContext = nullptr);

  //! Adds virtual trade to the last order.
  /** Raises an exception if order conditions will be violated by this trade.
   */
  void AddVirtualTrade(const Qty &, const Price &);

  const OrderTransactionContext &OpenAtMarketPrice();
  const OrderTransactionContext &OpenAtMarketPrice(const OrderParams &);
  const OrderTransactionContext &Open(const Price &);
  const OrderTransactionContext &Open(const Price &, const OrderParams &);
  const OrderTransactionContext &OpenImmediatelyOrCancel(const Price &);
  const OrderTransactionContext &OpenImmediatelyOrCancel(const Price &,
                                                         const OrderParams &);

  const OrderTransactionContext &CloseAtMarketPrice();
  const OrderTransactionContext &CloseAtMarketPrice(const OrderParams &);
  const OrderTransactionContext &Close(const Price &);
  const OrderTransactionContext &Close(const Price &, const Qty &maxQty);
  const OrderTransactionContext &Close(const Price &, const OrderParams &);
  const OrderTransactionContext &Close(const Price &,
                                       const Qty &maxQty,
                                       const OrderParams &);
  const OrderTransactionContext &CloseImmediatelyOrCancel(const Price &);
  const OrderTransactionContext &CloseImmediatelyOrCancel(const Price &,
                                                          const OrderParams &);

  //! Cancels all active orders.
  /** @sa IsCancelling
   * @return True if sent cancel request for one or more orders.
   */
  bool CancelAllOrders();
  //! Cancel request was sent by it is not executed yet.
  /** @sa CancelAllOrders
   */
  bool IsCancelling() const;

  StateUpdateConnection Subscribe(const StateUpdateSlot &) const;

  //! Runs each attached algorithm.
  /** @sa AttachAlgo
   * @sa Algo
   */
  void RunAlgos();

 protected:
  virtual boost::shared_ptr<const OrderTransactionContext> DoOpenAtMarketPrice(
      const Qty &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) = 0;
  virtual boost::shared_ptr<const OrderTransactionContext> DoOpen(
      const Qty &,
      const Price &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) = 0;
  virtual boost::shared_ptr<const OrderTransactionContext>
  DoOpenImmediatelyOrCancel(const Qty &,
                            const Price &,
                            const OrderParams &,
                            std::unique_ptr<OrderStatusHandler> &&) = 0;

  virtual boost::shared_ptr<const OrderTransactionContext> DoCloseAtMarketPrice(
      const Qty &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) = 0;
  virtual boost::shared_ptr<const OrderTransactionContext> DoClose(
      const Qty &,
      const Price &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) = 0;
  virtual boost::shared_ptr<const OrderTransactionContext>
  DoCloseImmediatelyOrCancel(const Qty &,
                             const Price &,
                             const OrderParams &,
                             std::unique_ptr<OrderStatusHandler> &&) = 0;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

//////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API LongPosition : public Position {
 public:
  explicit LongPosition(const boost::shared_ptr<Operation> &,
                        int64_t subOperationId,
                        Security &,
                        const Lib::Currency &,
                        const Qty &,
                        const Price &startPrice,
                        const Lib::TimeMeasurement::Milestones &);
  ~LongPosition() override;

  PositionSide GetSide() const override;
  OrderSide GetOpenOrderSide() const override;
  OrderSide GetCloseOrderSide() const override;

  Volume GetRealizedPnl() const override;
  Lib::Double GetRealizedPnlRatio() const override;
  Volume GetUnrealizedPnl() const override;

  Price GetMarketOpenPrice() const override;
  Price GetMarketClosePrice() const override;
  Price GetMarketOpenOppositePrice() const override;
  Price GetMarketCloseOppositePrice() const override;

 protected:
  boost::shared_ptr<const OrderTransactionContext> DoOpenAtMarketPrice(
      const Qty &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;
  boost::shared_ptr<const OrderTransactionContext> DoOpen(
      const Qty &,
      const Price &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;
  boost::shared_ptr<const OrderTransactionContext> DoOpenImmediatelyOrCancel(
      const Qty &,
      const Price &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;

  boost::shared_ptr<const OrderTransactionContext> DoCloseAtMarketPrice(
      const Qty &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;
  boost::shared_ptr<const OrderTransactionContext> DoClose(
      const Qty &,
      const Price &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;
  boost::shared_ptr<const OrderTransactionContext> DoCloseImmediatelyOrCancel(
      const Qty &,
      const Price &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;
};

//////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API ShortPosition : public Position {
 public:
  explicit ShortPosition(const boost::shared_ptr<Operation> &,
                         int64_t subOperationId,
                         Security &,
                         const Lib::Currency &,
                         const Qty &,
                         const Price &startPrice,
                         const Lib::TimeMeasurement::Milestones &);
  ~ShortPosition() override;

  PositionSide GetSide() const override;
  OrderSide GetOpenOrderSide() const override;
  OrderSide GetCloseOrderSide() const override;

  Volume GetRealizedPnl() const override;
  Lib::Double GetRealizedPnlRatio() const override;
  Volume GetUnrealizedPnl() const override;

  Price GetMarketOpenPrice() const override;
  Price GetMarketClosePrice() const override;
  Price GetMarketOpenOppositePrice() const override;
  Price GetMarketCloseOppositePrice() const override;

 protected:
  boost::shared_ptr<const OrderTransactionContext> DoOpenAtMarketPrice(
      const Qty &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;
  boost::shared_ptr<const OrderTransactionContext> DoOpen(
      const Qty &,
      const Price &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;
  boost::shared_ptr<const OrderTransactionContext> DoOpenImmediatelyOrCancel(
      const Qty &,
      const Price &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;

  boost::shared_ptr<const OrderTransactionContext> DoCloseAtMarketPrice(
      const Qty &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;
  boost::shared_ptr<const OrderTransactionContext> DoClose(
      const Qty &,
      const Price &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;
  boost::shared_ptr<const OrderTransactionContext> DoCloseImmediatelyOrCancel(
      const Qty &,
      const Price &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&) override;
};

//////////////////////////////////////////////////////////////////////////
}  // namespace trdk
