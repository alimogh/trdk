/*******************************************************************************
 *   Created: 2018/02/16 04:30:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Crex24Request.hpp"
#include "Crex24Util.hpp"
#include "NonceStorage.hpp"
#include "PollingTask.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class Crex24TradingSystem : public TradingSystem {
 public:
  typedef TradingSystem Base;

 private:
  struct Settings : Rest::Settings {
    std::string apiKey;
    std::vector<unsigned char> apiSecret;

    explicit Settings(const boost::property_tree::ptree &, ModuleEventsLog &);
  };

  class OrderTransactionContext : public trdk::OrderTransactionContext {
   public:
    typedef trdk::OrderTransactionContext Base;

    explicit OrderTransactionContext(TradingSystem &, OrderId &&);

    bool RegisterTrade(const std::string &);

   private:
    boost::unordered_set<std::string> m_trades;
  };

  class PrivateRequest : public Crex24Request {
   public:
    typedef Crex24Request Base;

    explicit PrivateRequest(const std::string &name,
                            const std::string &params,
                            const Settings &,
                            NonceStorage &,
                            bool isPriority,
                            const Context &,
                            ModuleEventsLog &,
                            ModuleTradingLog * = nullptr);
    ~PrivateRequest() override = default;

    Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

   protected:
    void PrepareRequest(const Poco::Net::HTTPClientSession &,
                        const std::string &body,
                        Poco::Net::HTTPRequest &) const override;
    bool IsPriority() const override;
    void CreateBody(const Poco::Net::HTTPClientSession &,
                    std::string &result) const override;

   private:
    const Settings &m_settings;
    const bool m_isPriority;
    NonceStorage &m_nonces;
    mutable boost::optional<NonceStorage::TakenValue> m_nonce;
  };

 public:
  explicit Crex24TradingSystem(const App &,
                               const TradingMode &,
                               Context &,
                               std::string instanceName,
                               std::string title,
                               const boost::property_tree::ptree &);
  ~Crex24TradingSystem() override = default;

  bool IsConnected() const override;

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

  std::unique_ptr<trdk::OrderTransactionContext> SendOrderTransaction(
      trdk::Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderParams &,
      const OrderSide &,
      const TimeInForce &) override;

  void SendCancelOrderTransaction(
      const trdk::OrderTransactionContext &) override;

  void OnTransactionSent(const trdk::OrderTransactionContext &) override;

 private:
  void UpdateBalances();

  void UpdateOrders();
  void UpdateOrder(OrderTransactionContext &,
                   const boost::posix_time::ptime &,
                   const boost::property_tree::ptree &);

  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;
  boost::unordered_map<std::string, Crex24Product> m_products;

  NonceStorage m_nonces;

  TradingLib::BalancesContainer m_balances;
  PrivateRequest m_balancesRequest;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPSClientSession> m_pollingSession;

  PollingTask m_pollingTask;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
