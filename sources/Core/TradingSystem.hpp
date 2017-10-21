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

  struct TradeInfo {
    std::string id;
    trdk::Qty qty;
    trdk::Price price;
  };

  typedef boost::function<void(const trdk::OrderId &,
                               const std::string &tradingSystemOrderId,
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

  class TRDK_CORE_API SendingError : public trdk::TradingSystem::Error {
   public:
    SendingError(const char *what) noexcept;
  };

  class TRDK_CORE_API OrderIsUnknown : public trdk::TradingSystem::Error {
   public:
    explicit OrderIsUnknown(const char *what) noexcept;
  };

  class TRDK_CORE_API UnknownOrderCancelError
      : public trdk::TradingSystem::OrderIsUnknown {
   public:
    explicit UnknownOrderCancelError(const char *what) noexcept;
  };

  class TRDK_CORE_API ConnectionDoesntExistError
      : public trdk::TradingSystem::Error {
   public:
    explicit ConnectionDoesntExistError(const char *what) noexcept;
  };

  //! Account is unknown or default account requested but hasn't been set.
  class TRDK_CORE_API UnknownAccountError : public trdk::TradingSystem::Error {
   public:
    explicit UnknownAccountError(const char *what) noexcept;
  };

  //! Broker position error.
  class TRDK_CORE_API PositionError : public trdk::TradingSystem::Error {
   public:
    explicit PositionError(const char *what) noexcept;
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
    * @throw trdk::TradingSystem::UnknownAccountError
    */
  virtual const trdk::TradingSystem::Account &GetAccount() const;

 public:
  trdk::OrderId SellAtMarketPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &,
      trdk::RiskControlScope &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
  trdk::OrderId Sell(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &,
      trdk::RiskControlScope &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
  trdk::OrderId SellImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &,
      trdk::RiskControlScope &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
  trdk::OrderId SellAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &,
      trdk::RiskControlScope &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);

  trdk::OrderId BuyAtMarketPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &,
      trdk::RiskControlScope &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
  trdk::OrderId Buy(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &,
      trdk::RiskControlScope &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
  trdk::OrderId BuyImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &,
      trdk::RiskControlScope &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);
  trdk::OrderId BuyAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &,
      trdk::RiskControlScope &,
      const trdk::Lib::TimeMeasurement::Milestones &strategyTimeMeasurement);

  void CancelOrder(const trdk::OrderId &);

  virtual void Test();

 public:
  virtual void OnSettingsUpdate(const trdk::Lib::IniSectionRef &);

 protected:
  virtual void CreateConnection(const trdk::Lib::IniSectionRef &) = 0;

 protected:
  virtual trdk::OrderId SendSellAtMarketPrice(trdk::Security &,
                                              const trdk::Lib::Currency &,
                                              const trdk::Qty &,
                                              const trdk::OrderParams &) = 0;
  virtual trdk::OrderId SendSellAtMarketPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&);

  virtual trdk::OrderId SendSell(trdk::Security &,
                                 const trdk::Lib::Currency &,
                                 const trdk::Qty &,
                                 const trdk::Price &,
                                 const trdk::OrderParams &) = 0;
  virtual trdk::OrderId SendSell(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&);

  virtual trdk::OrderId SendSellImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &) = 0;
  virtual trdk::OrderId SendSellImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&);

  virtual trdk::OrderId SendSellAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &) = 0;
  virtual trdk::OrderId SendSellAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&);

  virtual trdk::OrderId SendBuyAtMarketPrice(trdk::Security &,
                                             const trdk::Lib::Currency &,
                                             const trdk::Qty &,
                                             const trdk::OrderParams &) = 0;
  virtual trdk::OrderId SendBuyAtMarketPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&);

  virtual trdk::OrderId SendBuy(trdk::Security &,
                                const trdk::Lib::Currency &,
                                const trdk::Qty &,
                                const trdk::Price &,
                                const trdk::OrderParams &) = 0;
  virtual trdk::OrderId SendBuy(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&);

  virtual trdk::OrderId SendBuyImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &) = 0;
  virtual trdk::OrderId SendBuyImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&);

  virtual trdk::OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &) = 0;
  virtual trdk::OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const trdk::TradingSystem::OrderStatusUpdateSlot &&);

  virtual void SendCancelOrder(const trdk::OrderId &) = 0;

  //! Notifies trading system about order state change.
  /** Method is not thread-safe.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderStatusUpdate(const trdk::OrderId &,
                           const std::string &tradingSystemOrderId,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty,
                           const trdk::Volume &commission,
                           const trdk::TradingSystem::TradeInfo &);
  //! Notifies trading system about order state change.
  /** Method is not thread-safe.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderStatusUpdate(const trdk::OrderId &,
                           const std::string &tradingSystemOrderId,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty);
  //! Notifies trading system about order state change.
  /** Method is not thread-safe.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderStatusUpdate(const trdk::OrderId &,
                           const std::string &tradingSystemOrderId,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty,
                           const trdk::Volume &commission);
  //! Notifies trading system about order state change.
  /** Method is not thread-safe.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderStatusUpdate(const trdk::OrderId &,
                           const std::string &tradingSystemOrderId,
                           const trdk::OrderStatus &,
                           const trdk::Qty &remainingQty,
                           const trdk::TradingSystem::TradeInfo &);
  //! Notifies trading system about order error.
  /** Method is not thread-safe.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderError(const trdk::OrderId &,
                    const std::string &tradingSystemOrderId,
                    const std::string &&error);
  //! Notifies trading system about order reject.
  /** Method is not thread-safe.
    * @throw  OrderIsUnknown  Order handler is not registered.
    */
  void OnOrderReject(const trdk::OrderId &,
                     const std::string &tradingSystemOrderId,
                     const std::string &&reason);

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
  virtual ~LegacyTradingSystem() override;

 public:
  virtual trdk::OrderId SendSellAtMarketPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &) override;
  virtual trdk::OrderId SendSell(trdk::Security &,
                                 const trdk::Lib::Currency &,
                                 const trdk::Qty &,
                                 const trdk::Price &,
                                 const trdk::OrderParams &) override;
  virtual trdk::OrderId SendSellImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &) override;
  virtual trdk::OrderId SendSellAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &) override;
  virtual trdk::OrderId SendBuyAtMarketPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &) override;
  virtual trdk::OrderId SendBuy(trdk::Security &,
                                const trdk::Lib::Currency &,
                                const trdk::Qty &,
                                const trdk::Price &,
                                const trdk::OrderParams &) override;
  virtual trdk::OrderId SendBuyImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &) override;
  virtual trdk::OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &) override;
};

////////////////////////////////////////////////////////////////////////////////
}
