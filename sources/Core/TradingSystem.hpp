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
    size_t tradingSystemIndex,
    trdk::Context &,
    const std::string &instanceName,
    const trdk::Lib::IniSectionRef &);

struct TradingSystemAndMarketDataSourceFactoryResult {
  boost::shared_ptr<trdk::TradingSystem> tradingSystem;
  boost::shared_ptr<trdk::MarketDataSource> marketDataSource;
};
typedef trdk::TradingSystemAndMarketDataSourceFactoryResult(
    TradingSystemAndMarketDataSourceFactory)(const trdk::TradingMode &,
                                             size_t tradingSystemIndex,
                                             size_t marketDataSourceSystemIndex,
                                             trdk::Context &,
                                             const std::string &instanceName,
                                             const trdk::Lib::IniSectionRef &);

////////////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API TradingSystem : virtual public trdk::Interactor {
 public:
  typedef trdk::Interactor Base;

  typedef trdk::ModuleEventsLog Log;
  typedef trdk::ModuleTradingLog TradingLog;

  //! Trade.
  struct TradeInfo {
    //! Trade price. If set as "zero" - will be calculated automatically.
    trdk::Price price;
    //! Trade quantity. If set as "zero" - will be calculated automatically.
    trdk::Qty qty;
    //! Trade ID. Optional.
    boost::optional<std::string> id;
  };

  typedef boost::function<void(const trdk::OrderId &,
                               const trdk::OrderStatus &,
                               const trdk::Qty &remainingQty,
                               const boost::optional<trdk::Volume> &commission,
                               const trdk::TradingSystem::TradeInfo *tradeInfo)>
      OrderStatusUpdateSlot;

  struct Account {
    boost::atomic<double> cashBalance;
    boost::atomic<double> equityWithLoanValue;
    boost::atomic<double> maintenanceMargin;
    boost::atomic<double> excessLiquidity;

    Account()
        : cashBalance(.0),
          equityWithLoanValue(.0),
          maintenanceMargin(.0),
          excessLiquidity(.0) {}
  };

  struct Position {
    std::string account;
    trdk::Lib::Symbol symbol;
    trdk::Qty qty;

    Position() : qty(0) {}

    explicit Position(const std::string &account,
                      const Lib::Symbol &symbol,
                      const Qty &qty)
        : account(account), symbol(symbol), qty(qty) {}
  };

 public:
  class TRDK_CORE_API Error : public Base::Error {
   public:
    explicit Error(const char *what) noexcept;
  };

  class TRDK_CORE_API OrderParamsError : public trdk::TradingSystem::Error {
   public:
    explicit OrderParamsError(const char *what,
                              const trdk::Qty &,
                              const trdk::OrderParams &) noexcept;
  };

  class TRDK_CORE_API OrderIsUnknown : public trdk::TradingSystem::Error {
   public:
    explicit OrderIsUnknown(const char *what) noexcept;
  };

  class TRDK_CORE_API ConnectionDoesntExistError
      : public trdk::TradingSystem::Error {
   public:
    explicit ConnectionDoesntExistError(const char *what) noexcept;
  };

 public:
  explicit TradingSystem(const trdk::TradingMode &,
                         size_t index,
                         trdk::Context &,
                         const std::string &instanceName);
  virtual ~TradingSystem() override;

  TRDK_CORE_API friend std::ostream &operator<<(std::ostream &,
                                                const trdk::TradingSystem &);

 public:
  const trdk::TradingMode &GetMode() const;

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
  //! Returns default account info.
  /** All values is unscaled. If default account hasn't been set - throws
    * an exception.
    */
  virtual const trdk::TradingSystem::Account &GetAccount() const;

  std::vector<trdk::OrderId> GetActiveOrderList() const;

 public:
  boost::shared_ptr<const trdk::OrderTransactionContext> SendOrder(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &,
      trdk::RiskControlScope &,
      const trdk::OrderSide &,
      const trdk::TimeInForce &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyDelaysMeasurement);

  //! Cancels active orders.
  /** @return True, if order is known and cancel-comand suscessfully sent.
    *         False if order is unknown.
    */
  bool CancelOrder(const trdk::OrderId &);

 public:
  virtual void OnSettingsUpdate(const trdk::Lib::IniSectionRef &);

 protected:
  virtual void CreateConnection(const trdk::Lib::IniSectionRef &) = 0;

 protected:
  virtual std::unique_ptr<trdk::OrderTransactionContext> SendOrderTransaction(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::OrderParams &,
      const trdk::OrderSide &,
      const trdk::TimeInForce &) = 0;
  virtual boost::shared_ptr<trdk::OrderTransactionContext> SendOrderTransaction(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::OrderSide &,
      const trdk::TimeInForce &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&);

  virtual void SendCancelOrderTransaction(const trdk::OrderId &) = 0;

  virtual void OnTransactionSent(const trdk::OrderId &);

 protected:
  //! Notifies trading system about order state change.
  /** The method is not thread-safe. Each who will call it should provide
    * correct sequence of calls or do it from one thread.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderStatusUpdate(const trdk::OrderId &,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty,
                           const trdk::Volume &commission,
                           trdk::TradingSystem::TradeInfo &&);
  //! Notifies trading system about order state change.
  /** The method is not thread-safe. Each who will call it should provide
    * correct sequence of calls or do it from one thread.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderStatusUpdate(const trdk::OrderId &,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty);
  //! Notifies trading system about order state change.
  /** The method is not thread-safe. Each who will call it should provide
    * correct sequence of calls or do it from one thread.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderStatusUpdate(const trdk::OrderId &,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty,
                           const trdk::Volume &commission);
  //! Notifies trading system about order state change.
  /** The method is not thread-safe. Each who will call it should provide
    * correct sequence of calls or do it from one thread.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderStatusUpdate(const trdk::OrderId &,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty,
                           trdk::TradingSystem::TradeInfo &&);
  //! Notifies trading system about order state change.
  /** The method is not thread-safe. Each who will call it should provide
    * correct sequence of calls or do it from one thread.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderStatusUpdate(
      const trdk::OrderId &,
      const trdk::OrderStatus &,
      const trdk::Qty &remainingQty,
      const boost::function<void(trdk::OrderTransactionContext &)> &);
  //! Notifies trading system about order state change.
  /** The method is not thread-safe. Each who will call it should provide
    * correct sequence of calls or do it from one thread.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderStatusUpdate(
      const trdk::OrderId &,
      const trdk::OrderStatus &,
      const trdk::Qty &remainingQty,
      trdk::TradingSystem::TradeInfo &&,
      const boost::function<void(trdk::OrderTransactionContext &)> &);
  //! Notifies trading system about order canceling.
  void OnOrderCancel(const trdk::OrderId &);
  //! Notifies trading system about order error.
  /** The method is not thread-safe. Each who will call it should provide
    * correct sequence of calls or do it from one thread.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderError(const trdk::OrderId &, const std::string &&error);
  //! Notifies trading system about order reject.
  /** The method is not thread-safe. Each who will call it should provide
    * correct sequence of calls or do it from one thread.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderReject(const trdk::OrderId &, const std::string &&reason);

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

////////////////////////////////////////////////////////////////////////////////

class TRDK_CORE_API LegacyTradingSystem : public trdk::TradingSystem {
 public:
  explicit LegacyTradingSystem(const trdk::TradingMode &,
                               size_t index,
                               trdk::Context &,
                               const std::string &instanceName);
  virtual ~LegacyTradingSystem() override = default;

 protected:
  virtual std::unique_ptr<trdk::OrderTransactionContext> SendOrderTransaction(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::OrderParams &,
      const trdk::OrderSide &,
      const trdk::TimeInForce &) override;
  virtual boost::shared_ptr<trdk::OrderTransactionContext> SendOrderTransaction(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::OrderSide &,
      const trdk::TimeInForce &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&) override;

  virtual trdk::OrderId SendSellAtMarketPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&) = 0;
  virtual trdk::OrderId SendSell(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&) = 0;
  virtual trdk::OrderId SendSellImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&) = 0;
  virtual trdk::OrderId SendBuyAtMarketPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&) = 0;
  virtual trdk::OrderId SendBuy(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&) = 0;
  virtual trdk::OrderId SendBuyImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&) = 0;
};

////////////////////////////////////////////////////////////////////////////////
}
