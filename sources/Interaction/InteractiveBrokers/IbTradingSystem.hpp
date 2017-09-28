/**************************************************************************
 *   Created: May 26, 2012 9:44:37 PM
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Context.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradingSystem.hpp"
#include "IbSecurity.hpp"

namespace trdk {
namespace Interaction {
namespace InteractiveBrokers {

typedef std::vector<boost::function<void()>> OrderCallbackList;

class TradingSystem : public trdk::TradingSystem,
                      public trdk::MarketDataSource {
  friend class trdk::Interaction::InteractiveBrokers::Client;

 private:
  struct BySymbol {};
  struct ByOrder {};
  struct ByExecution {};

  typedef Concurrency::reader_writer_lock OrdersMutex;
  typedef OrdersMutex::scoped_lock OrdersWriteLock;
  typedef OrdersMutex::scoped_lock_read OrdersReadLock;

  typedef Concurrency::reader_writer_lock PositionsMutex;
  typedef PositionsMutex::scoped_lock_read PositionsReadLock;
  typedef PositionsMutex::scoped_lock PositionsWriteLock;
  struct Position : public trdk::TradingSystem::Position {
    typedef trdk::TradingSystem::Position Base;
    Position() = default;
    explicit Position(const std::string &account,
                      const Lib::Symbol &symbol,
                      Qty qty)
        : Base(account, symbol, qty) {}
    const std::string &GetSymbol() const { return symbol.GetSymbol(); }
    const Lib::Currency &GetCurrency() const { return symbol.GetCurrency(); }
  };

  struct PlacedOrder {
    struct CommissionRecord {
      std::string execId;
      boost::optional<trdk::Volume> commission;
    };
    struct FinalUpdate {
      int permOrderId;
      OrderStatus status;
      Qty filled;
      Qty remaining;
      double lastFillPrice;
    };

    OrderId id;
    trdk::Security *security;
    OrderStatusUpdateSlot callback;
    Qty filled;
    std::vector<CommissionRecord> commissions;
    boost::optional<FinalUpdate> finalUpdate;

    //! Updates field "filled".
    /** Should be "const" and PlacedOrder is a list item and filled
      * is not part of list key.
      */
    void UpdateFilled(const Qty &newValue) const {
      AssertLt(filled, newValue);
      const_cast<PlacedOrder *>(this)->filled = newValue;
    }

    bool HasUnreceviedCommission() const {
      for (const auto commission : commissions) {
        if (!commission.commission) {
          return true;
        }
      }
      return false;
    }

    Volume CalcCommission() const {
      Volume result = 0;
      for (const auto commission : commissions) {
        if (commission.commission) {
          result += *commission.commission;
        }
      }
      return result;
    }
  };

  typedef boost::multi_index_container<
      PlacedOrder,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_non_unique<
              boost::multi_index::tag<BySymbol>,
              boost::multi_index::member<PlacedOrder,
                                         trdk::Security *,
                                         &PlacedOrder::security>>,
          boost::multi_index::hashed_unique<
              boost::multi_index::tag<ByOrder>,
              boost::multi_index::
                  member<PlacedOrder, OrderId, &PlacedOrder::id>>>>
      PlacedOrderSet;

  struct Execution {
    std::string execId;
    trdk::OrderId orderId;
  };

  typedef boost::multi_index_container<
      Execution,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<
              boost::multi_index::tag<ByExecution>,
              boost::multi_index::
                  member<Execution, std::string, &Execution::execId>>,
          boost::multi_index::hashed_non_unique<
              boost::multi_index::tag<ByOrder>,
              boost::multi_index::
                  member<Execution, trdk::OrderId, &Execution::orderId>>>>
      ExecutionSet;

 public:
  explicit TradingSystem(const TradingMode &,
                         size_t tradingSystemIndex,
                         size_t marketDataSourceIndex,
                         Context &,
                         const std::string &tag,
                         const Lib::IniSectionRef &);
  virtual ~TradingSystem();

 public:
  using trdk::TradingSystem::GetContext;

  trdk::TradingSystem::Log &GetTsLog() const noexcept {
    return trdk::TradingSystem::GetLog();
  }

  trdk::MarketDataSource::Log &GetMdsLog() const noexcept {
    return trdk::MarketDataSource::GetLog();
  }

 public:
  virtual bool IsConnected() const override;
  virtual void Connect(const trdk::Lib::IniSectionRef &) override;

  virtual void SubscribeToSecurities() override;

 public:
  virtual const Account &GetAccount() const override;

 protected:
  virtual void CreateConnection(const trdk::Lib::IniSectionRef &) override;

  virtual trdk::Security &CreateNewSecurityObject(
      const trdk::Lib::Symbol &) override;

  virtual void SwitchToContract(
      trdk::Security &, const trdk::Lib::ContractExpiration &&) const override;

  virtual boost::optional<trdk::Lib::ContractExpiration> FindContractExpiration(
      const trdk::Lib::Symbol &, const boost::gregorian::date &) const override;

  virtual OrderId SendSellAtMarketPrice(trdk::Security &,
                                        const trdk::Lib::Currency &,
                                        const trdk::Qty &,
                                        const trdk::OrderParams &,
                                        const OrderStatusUpdateSlot &) override;
  virtual OrderId SendSell(trdk::Security &,
                           const trdk::Lib::Currency &,
                           const trdk::Qty &,
                           const trdk::Price &,
                           const trdk::OrderParams &,
                           const OrderStatusUpdateSlot &&) override;
  virtual OrderId SendSellAtMarketPriceWithStopPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;
  virtual OrderId SendSellImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;
  virtual OrderId SendSellAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;

  virtual OrderId SendBuyAtMarketPrice(trdk::Security &,
                                       const trdk::Lib::Currency &,
                                       const trdk::Qty &,
                                       const trdk::OrderParams &,
                                       const OrderStatusUpdateSlot &);
  virtual OrderId SendBuy(trdk::Security &,
                          const trdk::Lib::Currency &,
                          const trdk::Qty &,
                          const trdk::Price &,
                          const trdk::OrderParams &,
                          const OrderStatusUpdateSlot &&) override;
  virtual OrderId SendBuyAtMarketPriceWithStopPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &stopPrice,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;
  virtual OrderId SendBuyImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;
  virtual OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;

  virtual void SendCancelOrder(const OrderId &) override;

 private:
  trdk::OrderId RegOrder(PlacedOrder &&);

  void OnOrderStatus(const trdk::OrderId &,
                     int permOrderId,
                     const OrderStatus &,
                     const Qty &,
                     const Qty &,
                     double lastFillPrice,
                     OrderCallbackList &);
  void OnExecution(const OrderId &, const std::string &execId);
  void OnCommissionReport(const std::string &execId,
                          double commission,
                          OrderCallbackList &);

 private:
  const bool m_isTestSource;

  OrdersMutex m_ordersMutex;
  std::unique_ptr<Client> m_client;
  PlacedOrderSet m_placedOrders;
  ExecutionSet m_executionSet;

  std::vector<boost::shared_ptr<Security>> m_securities;
  mutable std::deque<boost::shared_ptr<Security>> m_unsubscribedSecurities;

  std::unique_ptr<Account> m_account;
};
}
}
}
