/*******************************************************************************
 *   Created: 2018/01/18 01:12:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "CexioRequest.hpp"
#include "CexioUtil.hpp"
#include "NonceStorage.hpp"
#include "PollingTask.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class CexioTradingSystem : public TradingSystem {
 public:
  typedef TradingSystem Base;

 private:
  struct Settings : public Rest::Settings {
    std::string username;
    std::string apiKey;
    std::string apiSecret;
    NonceStorage::Settings nonces;

    explicit Settings(const Lib::IniSectionRef &, ModuleEventsLog &);
  };

  class PrivateRequest : public CexioRequest {
   public:
    typedef CexioRequest Base;

   public:
    explicit PrivateRequest(const std::string &name,
                            const std::string &params,
                            const Settings &,
                            NonceStorage &nonces,
                            bool isPriority,
                            const Context &,
                            ModuleEventsLog &,
                            ModuleTradingLog * = nullptr);
    virtual ~PrivateRequest() override = default;

   public:
    virtual Response Send(Poco::Net::HTTPClientSession &);

   protected:
    virtual bool IsPriority() const override { return m_isPriority; }
    virtual void CreateBody(const Poco::Net::HTTPClientSession &,
                            std::string &result) const override;

   private:
    const Settings &m_settings;
    const bool m_isPriority;
    NonceStorage &m_nonces;
    mutable boost::optional<NonceStorage::TakenValue> m_nonce;
  };

  class BalancesRequest : public PrivateRequest {
   public:
    explicit BalancesRequest(const Settings &settings,
                             NonceStorage &nonces,
                             const Context &context,
                             ModuleEventsLog &);
  };

  class OrderRequest : public PrivateRequest {
   public:
    explicit OrderRequest(const std::string &name,
                          const std::string &params,
                          const Settings &settings,
                          NonceStorage &nonces,
                          const Context &context,
                          ModuleEventsLog &,
                          ModuleTradingLog &);
  };

 public:
  explicit CexioTradingSystem(const App &,
                              const TradingMode &,
                              Context &,
                              const std::string &instanceName,
                              const Lib::IniSectionRef &);
  virtual ~CexioTradingSystem() override = default;

 public:
  virtual bool IsConnected() const override { return !m_products.empty(); }

  virtual Balances &GetBalancesStorage() override { return m_balances; }

  virtual Volume CalcCommission(const Volume &,
                                const trdk::Security &) const override;

  virtual boost::optional<OrderCheckError> CheckOrder(
      const trdk::Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderSide &) const override;

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
  void UpdateOrders();
  void UpdateOrder(const OrderId &, const boost::property_tree::ptree &);

  boost::posix_time::ptime ParseTimeStamp(
      const std::string &key, const boost::property_tree::ptree &) const;

 private:
  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;
  mutable NonceStorage m_nonces;
  boost::unordered_map<std::string, CexioProduct> m_products;

  BalancesContainer m_balances;
  BalancesRequest m_balancesRequest;

  std::unique_ptr<Poco::Net::HTTPClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPClientSession> m_pollingSession;

  PollingTask m_pollingTask;
};
}
}
}
