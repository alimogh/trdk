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

typedef boost::shared_ptr<trdk::TradingSystem>(TradingSystemFactory)(
    const trdk::TradingMode &,
    trdk::Context &,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &);

struct TradingSystemAndMarketDataSourceFactoryResult {
  boost::shared_ptr<trdk::TradingSystem> tradingSystem;
  boost::shared_ptr<trdk::MarketDataSource> marketDataSource;
};
typedef trdk::TradingSystemAndMarketDataSourceFactoryResult(
    TradingSystemAndMarketDataSourceFactory)(const trdk::TradingMode &,
                                             trdk::Context &,
                                             const std::string &instanceName,
                                             const trdk::Lib::IniSectionRef &);

////////////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API TradingSystem : virtual public trdk::Interactor {
 public:
  typedef trdk::Interactor Base;

  typedef trdk::ModuleEventsLog Log;
  typedef trdk::ModuleTradingLog TradingLog;

  struct OrderCheckError {
    boost::optional<trdk::Qty> qty;
    boost::optional<trdk::Price> price;
    boost::optional<trdk::Volume> volume;
  };

 protected:
  template <trdk::Lib::Concurrency::Profile profile>
  struct ConcurrencyPolicyTrait {
    static_assert(profile == trdk::Lib::Concurrency::PROFILE_RELAX,
                  "Wrong concurrency profile");
    typedef boost::shared_mutex SharedMutex;
    typedef boost::mutex Mutex;
    typedef boost::shared_lock<SharedMutex> ReadLock;
    typedef boost::unique_lock<SharedMutex> WriteLock;
    typedef Mutex::scoped_lock Lock;
  };
  template <>
  struct ConcurrencyPolicyTrait<trdk::Lib::Concurrency::PROFILE_HFT> {
    typedef trdk::Lib::Concurrency::SpinMutex SharedMutex;
    typedef trdk::Lib::Concurrency::SpinMutex Mutex;
    typedef Mutex::ScopedLock WriteLock;
    typedef Mutex::ScopedLock ReadLock;
    typedef Mutex::ScopedLock Lock;
  };
  typedef ConcurrencyPolicyTrait<TRDK_CONCURRENCY_PROFILE> ConcurrencyPolicy;

  struct Order {
    trdk::Security &security;
    trdk::Lib::Currency currency;
    trdk::OrderSide side;
    std::unique_ptr<trdk::OrderStatusHandler> handler;
    trdk::Qty qty;
    trdk::Qty remainingQty;
    boost::optional<trdk::Price> price;
    trdk::Price actualPrice;
    bool isOpened;
    trdk::TimeInForce tif;
    trdk::Lib::TimeMeasurement::Milestones delayMeasurement;
    trdk::RiskControlScope &riskControlScope;
    boost::shared_ptr<const trdk::Position> position;
    trdk::RiskControlOperationId riskControlOperationId;
    std::unique_ptr<trdk::TimerScope> timerScope;
    boost::shared_ptr<trdk::OrderTransactionContext> transactionContext;
    bool isCancelRequestSent;
    size_t numberOfTrades;
    trdk::Lib::Double sumOfTradePrices;
    ConcurrencyPolicy::Mutex mutex;

    explicit Order(trdk::Security &,
                   const trdk::Lib::Currency &,
                   const trdk::OrderSide &,
                   std::unique_ptr<trdk::OrderStatusHandler> &&,
                   const trdk::Qty &qty,
                   const trdk::Qty &remainingQty,
                   const boost::optional<trdk::Price> &price,
                   trdk::Price actualPrice,
                   const trdk::TimeInForce &,
                   const trdk::Lib::TimeMeasurement::Milestones &,
                   trdk::RiskControlScope &,
                   boost::shared_ptr<const trdk::Position> &&);
  };

  typedef boost::unordered_map<trdk::OrderId,
                               boost::shared_ptr<trdk::TradingSystem::Order>>
      Orders;

 public:
  explicit TradingSystem(const trdk::TradingMode &,
                         trdk::Context &,
                         const std::string &instanceName);
  virtual ~TradingSystem() override;

  friend std::ostream &operator<<(std::ostream &oss,
                                  const trdk::TradingSystem &tradingSystem) {
    return oss << tradingSystem.GetStringId();
  }

 public:
  const trdk::TradingMode &GetMode() const;

  void AssignIndex(size_t);
  size_t GetIndex() const;

  trdk::Context &GetContext();
  const trdk::Context &GetContext() const;

  trdk::TradingSystem::Log &GetLog() const noexcept;
  trdk::TradingSystem::TradingLog &GetTradingLog() const noexcept;

  //! Identifies Trading System object by verbose name.
  /** Trading System instance name is unique, but can be empty.
   */
  const std::string &GetInstanceName() const;

  const std::string &GetStringId() const noexcept;

 public:
  virtual bool IsConnected() const = 0;
  void Connect(const trdk::Lib::IniSectionRef &);

 public:
  const boost::posix_time::time_duration &GetDefaultPollingInterval() const;

  const trdk::Balances &GetBalances() const;

  virtual trdk::Volume CalcCommission(const trdk::Qty &,
                                      const trdk::Price &,
                                      const trdk::OrderSide &,
                                      const trdk::Security &) const = 0;

  std::vector<trdk::OrderId> GetActiveOrderIdList() const;

 public:
  virtual boost::optional<trdk::TradingSystem::OrderCheckError> CheckOrder(
      const trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::OrderSide &) const;

  //! Returns true if the symbol is supported by the trading system.
  virtual bool CheckSymbol(const std::string &) const;

  //! Sends order synchronously.
  /** @return Order transaction pointer in any case.
   */
  boost::shared_ptr<const trdk::OrderTransactionContext> SendOrder(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&,
      trdk::RiskControlScope &,
      const trdk::OrderSide &,
      const trdk::TimeInForce &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyDelaysMeasurement);
  //! Sends position order synchronously.
  /** @return Order transaction pointer in any case.
   */
  boost::shared_ptr<const trdk::OrderTransactionContext> SendOrder(
      boost::shared_ptr<trdk::Position> &&,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::OrderParams &,
      std::unique_ptr<trdk::OrderStatusHandler> &&,
      const trdk::OrderSide &,
      const trdk::TimeInForce &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyDelaysMeasurement);

  //! Cancels active order synchronously.
  /** @return True, if order is known and cancel-command successfully sent.
   *         False if order is unknown.
   */
  bool CancelOrder(const trdk::OrderId &);

 public:
  virtual void OnSettingsUpdate(const trdk::Lib::IniSectionRef &);

 protected:
  virtual void CreateConnection(const trdk::Lib::IniSectionRef &) = 0;

  virtual trdk::Balances &GetBalancesStorage();

  //! Direct access to active order list.
  /** Maybe called only from overloaded methods SendOrderTransaction
   *  and SendCancelOrderTransaction.
   */
  const trdk::TradingSystem::Orders &GetActiveOrders() const;

 protected:
  virtual std::unique_ptr<trdk::OrderTransactionContext> SendOrderTransaction(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::OrderParams &,
      const trdk::OrderSide &,
      const trdk::TimeInForce &) = 0;

  virtual void SendCancelOrderTransaction(
      const trdk::OrderTransactionContext &) = 0;

  virtual void OnTransactionSent(const trdk::OrderTransactionContext &);

 protected:
  //! Reports order opened state.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderOpened(const boost::posix_time::ptime &, const trdk::OrderId &);
  //! Reports order opened state.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderOpened(
      const boost::posix_time::ptime &,
      const trdk::OrderId &,
      const boost::function<bool(trdk::OrderTransactionContext &)> &);

  //! Reports new trade. Doesn't finalize the order even no more remaining
  //! quantity is left.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnTrade(const boost::posix_time::ptime &,
               const trdk::OrderId &,
               trdk::Trade &&);
  //! Reports new trade. Doesn't finalize the order even no more remaining
  //! quantity is left.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnTrade(const boost::posix_time::ptime &,
               const trdk::OrderId &,
               trdk::Trade &&,
               const boost::function<bool(trdk::OrderTransactionContext &)> &);

  //! Finalize the order by filling.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderFilled(const boost::posix_time::ptime &,
                     const trdk::OrderId &,
                     const boost::optional<trdk::Volume> &commission);
  //! Finalize the order by filling.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderFilled(
      const boost::posix_time::ptime &,
      const trdk::OrderId &,
      const boost::optional<trdk::Volume> &commission,
      const boost::function<bool(trdk::OrderTransactionContext &)> &);
  //! Finalize the order by filling with trade info.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderFilled(const boost::posix_time::ptime &,
                     const trdk::OrderId &,
                     trdk::Trade &&,
                     const boost::optional<trdk::Volume> &commission);
  //! Finalize the order by filling with trade info.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderFilled(
      const boost::posix_time::ptime &,
      const trdk::OrderId &,
      trdk::Trade &&,
      const boost::optional<trdk::Volume> &commission,
      const boost::function<bool(trdk::OrderTransactionContext &)> &);

  //! Finalize the order by canceling.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderCanceled(const boost::posix_time::ptime &,
                       const trdk::OrderId &,
                       const boost::optional<trdk::Qty> &remainingQty,
                       const boost::optional<trdk::Volume> &commission);

  //! Finalize the order by rejecting.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderRejected(const boost::posix_time::ptime &,
                       const trdk::OrderId &,
                       const boost::optional<trdk::Qty> &remainingQty,
                       const boost::optional<trdk::Volume> &commission);

  //! Finalize the order by error.
  /** All order-events methods should be called from one thread or call should
   *  be synchronized. In another case - subscribers may receive notifications
   * in the wrong order.
   */
  void OnOrderError(const boost::posix_time::ptime &,
                    const trdk::OrderId &,
                    const boost::optional<trdk::Qty> &remainingQty,
                    const boost::optional<trdk::Volume> &commission,
                    const std::string &error);

  std::unique_ptr<trdk::OrderTransactionContext>
  SendOrderTransactionAndEmulateIoc(trdk::Security &,
                                    const trdk::Lib::Currency &,
                                    const trdk::Qty &,
                                    const boost::optional<trdk::Price> &,
                                    const trdk::OrderParams &,
                                    const trdk::OrderSide &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace trdk
