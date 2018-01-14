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

class TRDK_CORE_API Position
    : private boost::noncopyable,
      public boost::enable_shared_from_this<trdk::Position> {
 public:
  typedef void(StateUpdateSlotSignature)();
  typedef boost::function<StateUpdateSlotSignature> StateUpdateSlot;
  typedef boost::signals2::connection StateUpdateConnection;

  class TRDK_CORE_API Exception : public trdk::Lib::Exception {
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

 public:
  explicit Position(trdk::Strategy &,
                    trdk::TradingSystem &,
                    trdk::Security &,
                    const trdk::Lib::Currency &,
                    const trdk::Qty &,
                    const trdk::Price &startPrice,
                    const trdk::Lib::TimeMeasurement::Milestones &);
  explicit Position(const boost::shared_ptr<trdk::Operation> &,
                    int64_t subOperationId,
                    trdk::Strategy &,
                    trdk::Security &,
                    const trdk::Lib::Currency &,
                    const trdk::Qty &,
                    const trdk::Price &startPrice,
                    const trdk::Lib::TimeMeasurement::Milestones &);
  virtual ~Position();

 public:
  int64_t GetSubOperationId() const;

  //! Adds algorithm to the position.
  /** Will try to execute algorithm at each price update, but only if position
    * is not in the "canceling state".
    * @sa IsCancelling
    * @sa IsCompleted
    */
  void AddAlgo(std::unique_ptr<trdk::TradingLib::Algo> &&);
  //! Removes all algorithms.
  void RemoveAlgos();

  const boost::shared_ptr<trdk::Operation> &GetOperation();
  const boost::shared_ptr<const trdk::Operation> &GetOperation() const;

  template <typename Operation>
  Operation &GetTypedOperation() {
    return *boost::polymorphic_downcast<Operation *>(&*GetOperation());
  }
  template <typename Operation>
  const Operation &GetTypedOperation() const {
    return *boost::polymorphic_downcast<const Operation *>(&*GetOperation());
  }

  bool IsLong() const;
  virtual trdk::PositionSide GetSide() const = 0;
  virtual trdk::OrderSide GetOpenOrderSide() const = 0;
  virtual trdk::OrderSide GetCloseOrderSide() const = 0;

  const trdk::Lib::ContractExpiration &GetExpiration() const;

 public:
  const trdk::TradingSystem &GetTradingSystem() const;
  trdk::TradingSystem &GetTradingSystem();
  void ReplaceTradingSystem(trdk::Security &, trdk::TradingSystem &);

  const trdk::Strategy &GetStrategy() const noexcept;
  trdk::Strategy &GetStrategy() noexcept;

  const trdk::Security &GetSecurity() const noexcept;
  trdk::Security &GetSecurity() noexcept;

  const trdk::Lib::Currency &GetCurrency() const;

  const Lib::TimeMeasurement::Milestones &GetTimeMeasurement();

 public:
  const CloseReason &GetCloseReason() const noexcept;
  void SetCloseReason(const CloseReason &);
  void ResetCloseReason(const CloseReason & = CLOSE_REASON_NONE);

  //! Has opened quantity equals or more than planned quantity.
  /** @sa GetPlanedQty
    * @sa GetActiveQty
    * @sa IsOpened
    */
  bool IsFullyOpened() const;
  //! Has opened qty and doesn't have active open-orders.
  /** @sa IsClosed
    * @sa IsFullyOpened
    */
  bool IsOpened() const noexcept;
  //! Closed.
  /** First was opened, then closed, doesn't have active quantity and
    * active orders.
    * @sa IsOpened
    */
  bool IsClosed() const noexcept;

  //! Started.
  /** True if at least one of open-orders was sent.
  /** @sa IsCompleted
    */
  bool IsStarted() const noexcept;
  //! Started and now doesn't have any orders and active qty or market as
  //! completed.
  /** @sa IsStarted
    * @sa MarkAsCompleted
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
  //! Open operation started, but was rejected by trading system at opening or
  //! closing.
  bool IsRejected() const noexcept;

  bool HasActiveOrders() const noexcept;
  bool HasOpenedOrders() const noexcept;
  bool HasActiveOpenOrders() const noexcept;
  bool HasOpenedOpenOrders() const noexcept;
  bool HasActiveCloseOrders() const noexcept;
  bool HasOpenedCloseOrders() const noexcept;

 public:
  const trdk::Qty &GetPlanedQty() const;
  const trdk::Price &GetOpenStartPrice() const;
  //! Returns time of first order.
  /** Throws an exception if there is no open-order at this moment.
    * @sa GetCloseStartTime()
    * @sa GetActiveOrderTime()
    */
  const boost::posix_time::ptime &GetOpenStartTime() const;

  void SetOpenedQty(const trdk::Qty &) const noexcept;
  const trdk::Qty &GetOpenedQty() const noexcept;
  trdk::Price GetOpenAvgPrice() const;
  //! Returns price of active open-order.
  /** Throws an exception if there is no active open-order at this moment.
    * @sa GetActiveOrderPrice
    * @sa GetActiveCloseOrderPrice
    * @sa GetActiveOpenOrderTime
    * @return Active order price or boost::none if the order is an order by
    *         market price.
    */
  const boost::optional<trdk::Price> &GetActiveOpenOrderPrice() const;
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
  const trdk::Price &GetLastOpenTradePrice() const;
  trdk::Volume GetOpenedVolume() const;
  //! Time of first trade.
  const boost::posix_time::ptime &GetOpenTime() const;

  //! Returns position opening context if exists or throws an exception if
  //! doesn't have it.
  /** @sa GetNumberOfOpenOrders
    */
  const boost::shared_ptr<const trdk::OrderTransactionContext>
      &GetOpeningContext(size_t index) const;
  //! Returns position closing context if exists or throws an exception if
  //! doesn't have it.
  /** @sa GetNumberOfCloseOrders
    */
  const boost::shared_ptr<const trdk::OrderTransactionContext>
      &GetClosingContext(size_t index) const;

  trdk::Qty GetActiveQty() const noexcept;
  trdk::Volume GetActiveVolume() const;

  void SetCloseStartPrice(const trdk::Price &);
  const trdk::Price &GetCloseStartPrice() const;
  //! Returns time of first close-order.
  /** Throws an exception if there is no close-order at this moment.
    * @sa GetOpenStartTime()
    * @sa GetActiveCloseTime()
    */
  const boost::posix_time::ptime &GetCloseStartTime() const;
  trdk::Price GetCloseAvgPrice() const;
  //! Returns price of active close-order.
  /** Throws an exception if there is no active close-order at this moment.
    * @sa GetActiveOrderPrice
    * @sa GetActiveOpenOrderPrice
    * @sa GetActiveCloseOrderTime
    * @return Active order price or boost::none if the order is an order by
    *         market price.
    */
  const boost::optional<trdk::Price> &GetActiveCloseOrderPrice() const;
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
  const trdk::Price &GetLastCloseTradePrice() const;
  const trdk::Qty &GetClosedQty() const noexcept;
  void SetClosedQty(const trdk::Qty &);
  trdk::Volume GetClosedVolume() const;
  //! Time of last trade.
  const boost::posix_time::ptime &GetCloseTime() const;

  //! Returns price of active order.
  /** Throws an exception if there is no active order at this moment.
    * @sa GetActiveOpenOrderPrice
    * @sa GetActiveOpenClosePrice
    * @return Active order price or boost::none if the order is an order by
    *         market price.
    */
  const boost::optional<trdk::Price> &GetActiveOrderPrice() const;
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
  const trdk::Price &GetLastTradePrice() const;

  virtual trdk::Volume GetRealizedPnl() const = 0;
  trdk::Volume GetRealizedPnlVolume() const;
  //! Returns realized PnL ratio.
  /** @return If the value is 1.0 - no profit and no loss,
    *         if less then 1.0 - loss, if greater then 1.0 - profit.
    */
  virtual trdk::Lib::Double GetRealizedPnlRatio() const = 0;
  //! Returns percentage of profit.
  /** @return If the value is zero - no profit and no loss,
    *         if less then zero - loss, if greater then zero - profit.
    */
  trdk::Lib::Double GetRealizedPnlPercentage() const;
  virtual trdk::Volume GetUnrealizedPnl() const = 0;
  //! Realized PnL + unrealized PnL.
  trdk::Volume GetPlannedPnl() const;
  //! Returns true if position has profit by realized P&L.
  /** @return False if position has loss or does not have profit and does not
    *         have loss. True if position has profit.
    */
  bool IsProfit() const;

  trdk::Volume CalcCommission() const;

  virtual trdk::Price GetMarketOpenPrice() const = 0;
  virtual trdk::Price GetMarketClosePrice() const = 0;
  virtual trdk::Price GetMarketOpenOppositePrice() const = 0;
  virtual trdk::Price GetMarketCloseOppositePrice() const = 0;

  size_t GetNumberOfOpenOrders() const;
  size_t GetNumberOfOpenTrades() const;

  size_t GetNumberOfCloseOrders() const;
  size_t GetNumberOfCloseTrades() const;

 public:
  //! Restores position in open-state.
  /** Sets position in open state at current strategy, doesn't make any trading
    * actions.
    * @param[in] openPrice      Position opening price.
    * @param[in] openingContext Position opening context, if exists.
    */
  void RestoreOpenState(
      const boost::posix_time::ptime &openStartTime,
      const boost::posix_time::ptime &openTime,
      const trdk::Price &openPrice,
      const boost::shared_ptr<const trdk::OrderTransactionContext>
          &openingContext = nullptr);

 public:
  const trdk::OrderTransactionContext &OpenAtMarketPrice();
  const trdk::OrderTransactionContext &OpenAtMarketPrice(
      const trdk::OrderParams &);
  const trdk::OrderTransactionContext &Open(const trdk::Price &);
  const trdk::OrderTransactionContext &Open(const trdk::Price &,
                                            const trdk::OrderParams &);
  const trdk::OrderTransactionContext &OpenImmediatelyOrCancel(
      const trdk::Price &);
  const trdk::OrderTransactionContext &OpenImmediatelyOrCancel(
      const trdk::Price &, const trdk::OrderParams &);

 public:
  const trdk::OrderTransactionContext &CloseAtMarketPrice();
  const trdk::OrderTransactionContext &CloseAtMarketPrice(
      const trdk::OrderParams &);
  const trdk::OrderTransactionContext &Close(const trdk::Price &);
  const trdk::OrderTransactionContext &Close(const trdk::Price &,
                                             const trdk::Qty &maxQty);
  const trdk::OrderTransactionContext &Close(const trdk::Price &,
                                             const trdk::OrderParams &);
  const trdk::OrderTransactionContext &Close(const trdk::Price &,
                                             const trdk::Qty &maxQty,
                                             const trdk::OrderParams &);
  const trdk::OrderTransactionContext &CloseImmediatelyOrCancel(
      const trdk::Price &);
  const trdk::OrderTransactionContext &CloseImmediatelyOrCancel(
      const trdk::Price &, const trdk::OrderParams &);

 public:
  //! Cancels all active orders.
  /** @sa IsCancelling
    * @return True if sent cancel request for one or more orders.
    */
  bool CancelAllOrders();
  //! Cancel request was sent by it is not executed yet.
  /** @sa CancelAllOrders
    */
  bool IsCancelling() const;

 public:
  StateUpdateConnection Subscribe(const StateUpdateSlot &) const;

  //! Runs each attached algorithm.
  /** @sa AttachAlgo
    * @sa Algo
    */
  void RunAlgos();

  void ScheduleUpdateEvent(const boost::posix_time::time_duration &);

 protected:
  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoOpenAtMarketPrice(const trdk::Qty &,
                      const trdk::OrderParams &,
                      std::unique_ptr<trdk::OrderStatusHandler> &&) = 0;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext> DoOpen(
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&) = 0;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoOpenImmediatelyOrCancel(const trdk::Qty &,
                            const trdk::Price &,
                            const trdk::OrderParams &,
                            std::unique_ptr<trdk::OrderStatusHandler> &&) = 0;

  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoCloseAtMarketPrice(const trdk::Qty &,
                       const trdk::OrderParams &,
                       std::unique_ptr<trdk::OrderStatusHandler> &&) = 0;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext> DoClose(
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&) = 0;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoCloseImmediatelyOrCancel(const trdk::Qty &,
                             const trdk::Price &,
                             const trdk::OrderParams &,
                             std::unique_ptr<trdk::OrderStatusHandler> &&) = 0;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};

//////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API LongPosition : public Position {
 public:
  explicit LongPosition(trdk::Strategy &,
                        trdk::TradingSystem &,
                        trdk::Security &,
                        const trdk::Lib::Currency &,
                        const trdk::Qty &,
                        const trdk::Price &startPrice,
                        const trdk::Lib::TimeMeasurement::Milestones &);
  explicit LongPosition(const boost::shared_ptr<trdk::Operation> &,
                        int64_t subOperationId,
                        trdk::Strategy &,
                        trdk::Security &,
                        const trdk::Lib::Currency &,
                        const trdk::Qty &,
                        const trdk::Price &startPrice,
                        const trdk::Lib::TimeMeasurement::Milestones &);
  virtual ~LongPosition() override;

 public:
  virtual trdk::PositionSide GetSide() const override;
  virtual trdk::OrderSide GetOpenOrderSide() const override;
  virtual trdk::OrderSide GetCloseOrderSide() const override;

  virtual trdk::Volume GetRealizedPnl() const override;
  virtual trdk::Lib::Double GetRealizedPnlRatio() const override;
  virtual trdk::Volume GetUnrealizedPnl() const override;

  virtual trdk::Price GetMarketOpenPrice() const override;
  virtual trdk::Price GetMarketClosePrice() const override;
  virtual trdk::Price GetMarketOpenOppositePrice() const override;
  virtual trdk::Price GetMarketCloseOppositePrice() const override;

 protected:
  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoOpenAtMarketPrice(const trdk::Qty &,
                      const trdk::OrderParams &,
                      std::unique_ptr<trdk::OrderStatusHandler> &&) override;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext> DoOpen(
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&) override;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoOpenImmediatelyOrCancel(
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&) override;

  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoCloseAtMarketPrice(const trdk::Qty &,
                       const trdk::OrderParams &,
                       std::unique_ptr<trdk::OrderStatusHandler> &&) override;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext> DoClose(
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&) override;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoCloseImmediatelyOrCancel(
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&) override;
};

//////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API ShortPosition : public Position {
 public:
  explicit ShortPosition(trdk::Strategy &,
                         trdk::TradingSystem &,
                         trdk::Security &,
                         const trdk::Lib::Currency &,
                         const trdk::Qty &,
                         const trdk::Price &startPrice,
                         const trdk::Lib::TimeMeasurement::Milestones &);
  explicit ShortPosition(const boost::shared_ptr<trdk::Operation> &,
                         int64_t subOperationId,
                         trdk::Strategy &,
                         trdk::Security &,
                         const trdk::Lib::Currency &,
                         const trdk::Qty &,
                         const trdk::Price &startPrice,
                         const trdk::Lib::TimeMeasurement::Milestones &);
  virtual ~ShortPosition() override;

 public:
  virtual trdk::PositionSide GetSide() const override;
  virtual trdk::OrderSide GetOpenOrderSide() const override;
  virtual trdk::OrderSide GetCloseOrderSide() const override;

  virtual trdk::Volume GetRealizedPnl() const override;
  virtual trdk::Lib::Double GetRealizedPnlRatio() const override;
  virtual trdk::Volume GetUnrealizedPnl() const override;

  virtual trdk::Price GetMarketOpenPrice() const override;
  virtual trdk::Price GetMarketClosePrice() const override;
  virtual trdk::Price GetMarketOpenOppositePrice() const override;
  virtual trdk::Price GetMarketCloseOppositePrice() const override;

 protected:
  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoOpenAtMarketPrice(const trdk::Qty &,
                      const trdk::OrderParams &,
                      std::unique_ptr<trdk::OrderStatusHandler> &&) override;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext> DoOpen(
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&) override;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoOpenImmediatelyOrCancel(
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&) override;

  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoCloseAtMarketPrice(const trdk::Qty &,
                       const trdk::OrderParams &,
                       std::unique_ptr<trdk::OrderStatusHandler> &&) override;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext> DoClose(
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&) override;
  virtual boost::shared_ptr<const trdk::OrderTransactionContext>
  DoCloseImmediatelyOrCancel(
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&) override;
};

//////////////////////////////////////////////////////////////////////////
}
