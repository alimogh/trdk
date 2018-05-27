/*******************************************************************************
 *   Created: 2017/12/19 21:00:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "LivecoinRequest.hpp"
#include "LivecoinUtil.hpp"
#include "PollingTask.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class LivecoinTradingSystem : public TradingSystem {
 public:
  typedef TradingSystem Base;

 private:
  struct Settings : Rest::Settings {
    std::string apiKey;
    std::string apiSecret;

    explicit Settings(const boost::property_tree::ptree &, ModuleEventsLog &);
  };

  class PrivateRequest : public LivecoinRequest {
   public:
    explicit PrivateRequest(const std::string &name,
                            const std::string &method,
                            const Settings &,
                            const std::string &params,
                            const Context &,
                            ModuleEventsLog &,
                            ModuleTradingLog * = nullptr);
    ~PrivateRequest() override = default;

   protected:
    void PrepareRequest(const Poco::Net::HTTPClientSession &,
                        const std::string &,
                        Poco::Net::HTTPRequest &) const override;

   private:
    const Settings &m_settings;
  };

  class TradingRequest : public PrivateRequest {
   public:
    explicit TradingRequest(const std::string &name,
                            const Settings &,
                            const std::string &params,
                            const Context &,
                            ModuleEventsLog &,
                            ModuleTradingLog * = nullptr);
    ~TradingRequest() override = default;

    Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

   protected:
    bool IsPriority() const override { return true; }
  };

  class BalancesRequest : public PrivateRequest {
   public:
    explicit BalancesRequest(const Settings &,
                             const Context &,
                             ModuleEventsLog &);
    ~BalancesRequest() override = default;

   protected:
    bool IsPriority() const override { return false; }
  };

 public:
  explicit LivecoinTradingSystem(const App &,
                                 const TradingMode &,
                                 Context &,
                                 const std::string &instanceName,
                                 const boost::property_tree::ptree &);
  ~LivecoinTradingSystem() override = default;

  bool IsConnected() const override { return !m_products.empty(); }

  Balances &GetBalancesStorage() override { return m_balances; }

  Volume CalcCommission(const Qty &,
                        const Price &,
                        const OrderSide &,
                        const trdk::Security &) const override;

  boost::optional<OrderCheckError> CheckOrder(const trdk::Security &,
                                              const Lib::Currency &,
                                              const Qty &,
                                              const boost::optional<Price> &,
                                              const OrderSide &) const override;

  bool CheckSymbol(const std::string &) const override;

 protected:
  void CreateConnection() override;

  std::unique_ptr<OrderTransactionContext> SendOrderTransaction(
      trdk::Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderParams &,
      const OrderSide &,
      const TimeInForce &) override;

  void SendCancelOrderTransaction(const OrderTransactionContext &) override;

  void OnTransactionSent(const OrderTransactionContext &) override;

 private:
  void UpdateBalances();
  void UpdateOrders();

  Settings m_settings;
  boost::unordered_map<std::string, LivecoinProduct> m_products;

  TradingLib::BalancesContainer m_balances;
  BalancesRequest m_balancesRequest;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPSClientSession> m_pollingSession;

  PollingTask m_pollingTask;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk