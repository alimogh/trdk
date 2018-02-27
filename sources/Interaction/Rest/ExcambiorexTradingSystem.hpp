/*******************************************************************************
 *   Created: 2018/02/11 21:47:56
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "ExcambiorexRequest.hpp"
#include "ExcambiorexUtil.hpp"
#include "PollingTask.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class ExcambiorexTradingSystem : public TradingSystem {
 public:
  typedef TradingSystem Base;

 private:
  struct Settings : public Rest::Settings {
    std::string apiKey;
    std::string apiSecret;

    explicit Settings(const Lib::IniSectionRef &, ModuleEventsLog &);
  };

  class PrivateRequest : public ExcambiorexRequest {
   public:
    typedef ExcambiorexRequest Base;

   public:
    explicit PrivateRequest(const std::string &name,
                            const std::string &params,
                            const Settings &,
                            bool isPriority,
                            const Context &,
                            ModuleEventsLog &,
                            ModuleTradingLog * = nullptr);
    virtual ~PrivateRequest() override = default;

   protected:
    virtual void CreateBody(const Poco::Net::HTTPClientSession &,
                            std::string &) const override;
    virtual bool IsPriority() const override;

   private:
    const Settings &m_settings;
    const bool m_isPriority;
  };

  class BalancesRequest : public PrivateRequest {
   public:
    explicit BalancesRequest(const Settings &settings,
                             const Context &context,
                             ModuleEventsLog &);
  };

  class OrderRequest : public PrivateRequest {
   public:
    explicit OrderRequest(const std::string &name,
                          const std::string &params,
                          const Settings &settings,
                          const Context &context,
                          ModuleEventsLog &,
                          ModuleTradingLog &);
  };

  class ActiveOrdersRequest : public PrivateRequest {
   public:
    explicit ActiveOrdersRequest(const std::string &params,
                                 const Settings &,
                                 const Context &,
                                 ModuleEventsLog &,
                                 ModuleTradingLog &);
  };

 public:
  explicit ExcambiorexTradingSystem(const App &,
                                    const TradingMode &,
                                    Context &,
                                    const std::string &instanceName,
                                    const Lib::IniSectionRef &);
  virtual ~ExcambiorexTradingSystem() override = default;

 public:
  virtual bool IsConnected() const override;

  virtual Balances &GetBalancesStorage() override { return m_balances; }

  virtual Volume CalcCommission(const trdk::Qty &,
                                const trdk::Price &,
                                const trdk::OrderSide &,
                                const trdk::Security &) const override;

  virtual boost::optional<OrderCheckError> CheckOrder(
      const trdk::Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderSide &) const override;

  virtual bool CheckSymbol(const std::string &) const override;

 protected:
  virtual void CreateConnection(const trdk::Lib::IniSectionRef &) override;

  virtual std::unique_ptr<trdk::OrderTransactionContext> SendOrderTransaction(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const boost::optional<trdk::Price> &,
      const trdk::OrderParams &,
      const trdk::OrderSide &,
      const trdk::TimeInForce &) override;

  virtual void SendCancelOrderTransaction(
      const OrderTransactionContext &) override;

  virtual void OnTransactionSent(const OrderTransactionContext &) override;

 private:
  void UpdateBalances();

  bool UpdateOrders();
  void FinalizeOrder(const boost::posix_time::ptime &, const OrderId &);

  void SetBalances(const boost::property_tree::ptree &);

  void SubsctibeAtOrderUpdates(const ExcambiorexProduct &);

 private:
  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;
  ExcambiorexProductList m_products;
  boost::unordered_map<std::string, std::string> m_currencies;

  TradingLib::BalancesContainer m_balances;
  BalancesRequest m_balancesRequest;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPSClientSession> m_pollingSession;

  boost::mutex m_ordersRequestMutex;
  boost::unordered_set<const ExcambiorexProduct *> m_orderRequestList;
  size_t m_orderRequestListVersion;

  boost::mutex m_orderCancelingMutex;
  boost::unordered_set<OrderId> m_canceledOrders;

  PollingTask m_pollingTask;
};
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
