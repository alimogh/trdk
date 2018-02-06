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
  struct Order {
    trdk::Security &security;
    trdk::Lib::Currency currency;
    trdk::OrderSide side;
    std::unique_ptr<trdk::OrderStatusHandler> handler;
    trdk::Qty qty;
    trdk::Qty remainingQty;
    boost::optional<trdk::Price> price;
    trdk::Price actualPrice;
    trdk::OrderStatus status;
    trdk::TimeInForce tif;
    trdk::Lib::TimeMeasurement::Milestones delayMeasurement;
    trdk::RiskControlScope &riskControlScope;
    boost::shared_ptr<const trdk::Position> position;
    trdk::RiskControlOperationId riskControlOperationId;
    boost::posix_time::ptime updateTime;
    std::unique_ptr<trdk::TimerScope> timerScope;
    boost::shared_ptr<trdk::OrderTransactionContext> transactionContext;
    bool isCancelRequestSent;

    bool IsChanged(const trdk::OrderStatus &,
                   const boost::optional<trdk::Qty> &,
                   const boost::optional<trdk::Trade> &) const;
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

  virtual trdk::Volume CalcCommission(const trdk::Volume &,
                                      const trdk::Security &) const;

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
  //! Notifies trading system about order status change.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderStatusUpdate(const boost::posix_time::ptime &,
                           const trdk::OrderId &,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty,
                           trdk::Trade &&,
                           const trdk::Volume &commission);
  //! Notifies trading system about order status change.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderStatusUpdate(const boost::posix_time::ptime &,
                           const trdk::OrderId &,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty);
  //! Notifies trading system about order status change.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderStatusUpdate(const boost::posix_time::ptime &,
                           const trdk::OrderId &,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty,
                           const trdk::Volume &commission);
  //! Notifies trading system about order status change.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderStatusUpdate(const boost::posix_time::ptime &,
                           const trdk::OrderId &,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty,
                           trdk::Trade &&);
  //! Notifies trading system about order status change.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderStatusUpdate(
      const boost::posix_time::ptime &,
      const trdk::OrderId &,
      const trdk::OrderStatus &,
      const trdk::Qty &remainingQty,
      const boost::function<bool(trdk::OrderTransactionContext &)> &);
  //! Notifies trading system about order status change.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderStatusUpdate(
      const boost::posix_time::ptime &,
      const trdk::OrderId &,
      const trdk::OrderStatus &,
      const trdk::Qty &remainingQty,
      trdk::Trade &&,
      const boost::function<bool(trdk::OrderTransactionContext &)> &);
  //! Notifies trading system about order status change.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderStatusUpdate(const boost::posix_time::ptime &,
                           const trdk::OrderId &,
                           const trdk::OrderStatus &);
  //! Notifies trading system about order status change.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderStatusUpdate(const boost::posix_time::ptime &,
                           const trdk::OrderId &,
                           const trdk::OrderStatus &,
                           trdk::Trade &&);
  //! Notifies trading system about order status change.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderStatusUpdate(
      const boost::posix_time::ptime &,
      const trdk::OrderId &,
      const trdk::OrderStatus &,
      trdk::Trade &&,
      const boost::function<bool(trdk::OrderTransactionContext &)> &);
  //! Notifies trading system about order status change.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderStatusUpdate(const boost::posix_time::ptime &,
                           const trdk::OrderId &,
                           const trdk::OrderStatus &,
                           trdk::Trade &&,
                           const trdk::Volume &commission);
  //! Notifies trading system about order canceling.
  void OnOrderCancel(const boost::posix_time::ptime &, const trdk::OrderId &);
  //! Notifies trading system about order canceling.
  void OnOrderCancel(const boost::posix_time::ptime &,
                     const trdk::OrderId &,
                     const trdk::Qty &remainingQty);
  //! Notifies trading system about order error.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderError(const boost::posix_time::ptime &,
                    const trdk::OrderId &,
                    const std::string &&error);
  //! Notifies trading system about order reject.
  /** The method is not thread-safe. Each who will call it should provide
   * correct sequence of calls or do it from one thread.
   * @throw  OrderIsUnknown  Order handler is not registered.
   */
  void OnOrderReject(const boost::posix_time::ptime &,
                     const trdk::OrderId &,
                     const std::string &&reason);

  //! General order update notification.
  /** May be used for any order. The method is not thread-safe. Each who will
   * call it should provide correct sequence of calls or do it from one thread.
   */
  void OnOrder(const trdk::OrderId &,
               const std::string &symbol,
               const trdk::OrderStatus &,
               const trdk::Qty &qty,
               const trdk::Qty &remainingQty,
               const boost::optional<trdk::Price> &,
               const trdk::OrderSide &,
               const trdk::TimeInForce &,
               const boost::posix_time::ptime &openTime,
               const boost::posix_time::ptime &updateTime);

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
