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
  struct Settings : public Rest::Settings {
    std::string apiKey;
    std::vector<unsigned char> apiSecret;

    explicit Settings(const Lib::IniSectionRef &, ModuleEventsLog &);
  };

  class OrderTransactionContext : public trdk::OrderTransactionContext {
   public:
    typedef trdk::OrderTransactionContext Base;

   public:
    explicit OrderTransactionContext(TradingSystem &, const OrderId &&);

   public:
    bool RegisterTrade(const std::string &);

   private:
    boost::unordered_set<std::string> m_trades;
  };

  class PrivateRequest : public Crex24Request {
   public:
    typedef Crex24Request Base;

   public:
    explicit PrivateRequest(const std::string &name,
                            const std::string &params,
                            const Settings &,
                            NonceStorage &,
                            bool isPriority,
                            const Context &,
                            ModuleEventsLog &,
                            ModuleTradingLog * = nullptr);
    virtual ~PrivateRequest() override = default;

   public:
    virtual Response Send(
        std::unique_ptr<Poco::Net::HTTPSClientSession> &) override;

   protected:
    virtual void PrepareRequest(const Poco::Net::HTTPClientSession &,
                                const std::string &body,
                                Poco::Net::HTTPRequest &) const override;
    virtual bool IsPriority() const override;
    virtual void CreateBody(const Poco::Net::HTTPClientSession &,
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
                               const std::string &instanceName,
                               const Lib::IniSectionRef &);
  virtual ~Crex24TradingSystem() override = default;

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
      const trdk::OrderTransactionContext &) override;

  virtual void OnTransactionSent(
      const trdk::OrderTransactionContext &) override;

 private:
  void UpdateBalances();

  void UpdateOrders();
  void UpdateOrder(OrderTransactionContext &,
                   const boost::posix_time::ptime &,
                   const boost::property_tree::ptree &);

 private:
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
