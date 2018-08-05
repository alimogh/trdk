/**************************************************************************
 *   Created: May 21, 2012 5:46:08 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Api.h"
#include "Interactor.hpp"

namespace trdk {

////////////////////////////////////////////////////////////////////////////////

struct TradingSystemAndMarketDataSourceFactoryResult {
  boost::shared_ptr<TradingSystem> tradingSystem;
  boost::shared_ptr<MarketDataSource> marketDataSource;
};
typedef TradingSystemAndMarketDataSourceFactoryResult(
    TradingSystemAndMarketDataSourceFactory)(
    const TradingMode &,
    Context &,
    std::string instanceName,
    std::string title,
    const boost::property_tree::ptree &);

////////////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API TradingSystem : virtual public Interactor {
 public:
  typedef Interactor Base;

  typedef ModuleEventsLog Log;
  typedef ModuleTradingLog TradingLog;

 protected:
  template <Lib::Concurrency::Profile profile>
  struct ConcurrencyPolicyTrait {
    static_assert(profile == Lib::Concurrency::PROFILE_RELAX,
                  "Wrong concurrency profile");
    typedef boost::shared_mutex SharedMutex;
    typedef boost::mutex Mutex;
    typedef boost::shared_lock<SharedMutex> ReadLock;
    typedef boost::unique_lock<SharedMutex> WriteLock;
    typedef Mutex::scoped_lock Lock;
  };
  template <>
  struct ConcurrencyPolicyTrait<Lib::Concurrency::PROFILE_HFT> {
    typedef Lib::Concurrency::SpinMutex SharedMutex;
    typedef Lib::Concurrency::SpinMutex Mutex;
    typedef Mutex::ScopedLock WriteLock;
    typedef Mutex::ScopedLock ReadLock;
    typedef Mutex::ScopedLock Lock;
  };
  typedef ConcurrencyPolicyTrait<TRDK_CONCURRENCY_PROFILE> ConcurrencyPolicy;

  struct Order {
    Security &security;
    Lib::Currency currency;
    OrderSide side;
    std::unique_ptr<OrderStatusHandler> handler;
    Qty qty;
    Qty remainingQty;
    boost::optional<Price> price;
    Price actualPrice;
    bool isOpened;
    TimeInForce tif;
    Lib::TimeMeasurement::Milestones delayMeasurement;
    RiskControlScope &riskControlScope;
    boost::shared_ptr<const Position> position;
    RiskControlOperationId riskControlOperationId;
    std::unique_ptr<TimerScope> timerScope;
    boost::shared_ptr<OrderTransactionContext> transactionContext;
    bool isCancelRequestSent;
    size_t numberOfTrades;
    Lib::Double sumOfTradePrices;
    ConcurrencyPolicy::Mutex mutex;

    explicit Order(Security &,
                   const Lib::Currency &,
                   const OrderSide &,
                   std::unique_ptr<OrderStatusHandler> &&,
                   const Qty &qty,
                   const Qty &remainingQty,
                   const boost::optional<Price> &price,
                   Price actualPrice,
                   const TimeInForce &,
                   const Lib::TimeMeasurement::Milestones &,
                   RiskControlScope &,
                   boost::shared_ptr<const Position> &&);
  };

  typedef boost::unordered_map<OrderId, boost::shared_ptr<Order>> Orders;

 public:
  explicit TradingSystem(const TradingMode &,
                         Context &,
                         std::string instanceName,
                         std::string title);
  TradingSystem(TradingSystem &&);
  TradingSystem(const TradingSystem &) = delete;
  TradingSystem &operator=(TradingSystem &&) = delete;
  TradingSystem &operator=(const TradingSystem &) = delete;
  ~TradingSystem() override;

  friend std::ostream &operator<<(std::ostream &oss,
                                  const TradingSystem &tradingSystem) {
    return oss << tradingSystem.GetStringId();
  }

  const TradingMode &GetMode() const;

  void AssignIndex(size_t);
  size_t GetIndex() const;

  Context &GetContext();
  const Context &GetContext() const;

  Log &GetLog() const noexcept;
  TradingLog &GetTradingLog() const noexcept;

  //! Identifies Trading System object by verbose name.
  /** Trading System instance name is unique, but can be empty.
   */
  const std::string &GetInstanceName() const;

  const std::string &GetTitle() const;

  const std::string &GetStringId() const noexcept;

  virtual bool IsConnected() const = 0;
  void Connect();

  const boost::posix_time::time_duration &GetDefaultPollingInterval() const;

  const Balances &GetBalances() const;

  virtual Volume CalcCommission(const Qty &,
                                const Price &,
                                const OrderSide &,
                                const Security &) const = 0;

  std::vector<boost::shared_ptr<const OrderTransactionContext>>
  GetActiveOrderContextList() const;

  virtual boost::optional<OrderCheckError> CheckOrder(
      const Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderSide &) const;

  //! Returns true if the symbol is supported by the trading system.
  virtual bool CheckSymbol(const std::string &) const;

  //! Sends order synchronously.
  /** @return Order transaction pointer in any case.
   */
  boost::shared_ptr<const OrderTransactionContext> SendOrder(
      Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&,
      RiskControlScope &,
      const OrderSide &,
      const TimeInForce &,
      const Lib::TimeMeasurement::Milestones &strategyDelaysMeasurement);
  //! Sends position order synchronously.
  /** @return Order transaction pointer in any case.
   */
  boost::shared_ptr<const OrderTransactionContext> SendOrder(
      boost::shared_ptr<Position> &&,
      const Qty &,
      const boost::optional<Price> &,
      const OrderParams &,
      std::unique_ptr<OrderStatusHandler> &&,
      const OrderSide &,
      const TimeInForce &,
      const Lib::TimeMeasurement::Milestones &strategyDelaysMeasurement);

  //! Cancels active order synchronously.
  /**
   * @return True, if order is known and cancel-command successfully sent.
   *         False if order is unknown.
   */
  bool CancelOrder(const OrderId &);

 protected:
  virtual void CreateConnection() = 0;

  virtual Balances &GetBalancesStorage();

  //! Returns active orders list.
  /** Call is thread-safe.
   */
  std::vector<boost::shared_ptr<OrderTransactionContext>>
  GetActiveOrderContextList();

  //! Direct access to active order list.
  /**
   * Maybe called only from overloaded methods SendOrderTransaction and
   * SendCancelOrderTransaction.
   */
  const Orders &GetActiveOrders() const;

  virtual std::unique_ptr<OrderTransactionContext> SendOrderTransaction(
      Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderParams &,
      const OrderSide &,
      const TimeInForce &) = 0;

  virtual void SendCancelOrderTransaction(const OrderTransactionContext &) = 0;

  virtual void OnTransactionSent(const OrderTransactionContext &);

  //! Reports order opened state.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderOpened(const boost::posix_time::ptime &, const OrderId &);
  //! Reports order opened state.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderOpened(const boost::posix_time::ptime &,
                     const OrderId &,
                     const boost::function<bool(OrderTransactionContext &)> &);

  //! Reports new trade. Doesn't finalize the order even no more remaining
  //! quantity is left.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnTrade(const boost::posix_time::ptime &, const OrderId &, Trade &&);
  //! Reports new trade. Doesn't finalize the order even no more remaining
  //! quantity is left.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnTrade(const boost::posix_time::ptime &,
               const OrderId &,
               Trade &&,
               const boost::function<bool(OrderTransactionContext &)> &);

  //! Reports about remaining quantity update if it was changed.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderRemainingQtyUpdated(const boost::posix_time::ptime &,
                                  const OrderId &,
                                  const Qty &remainingQty);
  //! Reports about remaining quantity update if it was changed.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderRemainingQtyUpdated(
      const boost::posix_time::ptime &,
      const OrderId &,
      const Qty &remainingQty,
      const boost::function<bool(OrderTransactionContext &)> &);

  //! Finalizes the order with automatically chosen status.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderCompleted(const boost::posix_time::ptime &,
                        const OrderId &,
                        const boost::optional<Volume> &commission);

  //! Finalizes the order by filling.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderFilled(const boost::posix_time::ptime &,
                     const OrderId &,
                     const boost::optional<Volume> &commission);
  //! Finalizes the order by filling.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderFilled(const boost::posix_time::ptime &,
                     const OrderId &,
                     const boost::optional<Volume> &commission,
                     const boost::function<bool(OrderTransactionContext &)> &);
  //! Finalizes the order by filling with trade info.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderFilled(const boost::posix_time::ptime &,
                     const OrderId &,
                     Trade &&,
                     const boost::optional<Volume> &commission);
  //! Finalizes the order by filling with trade info.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderFilled(const boost::posix_time::ptime &,
                     const OrderId &,
                     Trade &&,
                     const boost::optional<Volume> &commission,
                     const boost::function<bool(OrderTransactionContext &)> &);

  //! Finalizes the order by canceling.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderCanceled(const boost::posix_time::ptime &,
                       const OrderId &,
                       const boost::optional<Qty> &remainingQty,
                       const boost::optional<Volume> &commission);

  //! Finalizes the order by rejecting.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderRejected(const boost::posix_time::ptime &,
                       const OrderId &,
                       const boost::optional<Qty> &remainingQty,
                       const boost::optional<Volume> &commission);

  //! Finalizes the order by error.
  /**
   * All order-events methods should be called from one thread or call should
   * be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderError(const boost::posix_time::ptime &,
                    const OrderId &,
                    const boost::optional<Qty> &remainingQty,
                    const boost::optional<Volume> &commission,
                    const std::string &error);

  std::unique_ptr<OrderTransactionContext> SendOrderTransactionAndEmulateIoc(
      Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderParams &,
      const OrderSide &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace trdk
