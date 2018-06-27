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
  struct Settings : Rest::Settings {
    std::string username;
    std::string apiKey;
    std::string apiSecret;

    explicit Settings(const boost::property_tree::ptree &, ModuleEventsLog &);
  };

  class PrivateRequest : public CexioRequest {
   public:
    typedef CexioRequest Base;

   public:
    explicit PrivateRequest(const std::string &name,
                            const std::string &params,
                            const Settings &,
                            NonceStorage::TakenValue &&,
                            bool isPriority,
                            const Context &,
                            ModuleEventsLog &,
                            ModuleTradingLog * = nullptr);
    virtual ~PrivateRequest() override = default;

   public:
    virtual Response Send(std::unique_ptr<Poco::Net::HTTPSClientSession> &);

   protected:
    virtual bool IsPriority() const override { return m_isPriority; }
    virtual void CreateBody(const Poco::Net::HTTPClientSession &,
                            std::string &result) const override;

   private:
    const Settings &m_settings;
    const bool m_isPriority;
    NonceStorage::TakenValue m_nonce;
  };

  class BalancesRequest : public PrivateRequest {
   public:
    explicit BalancesRequest(const Settings &settings,
                             NonceStorage::TakenValue &&,
                             const Context &context,
                             ModuleEventsLog &);
  };

  class OrderRequest : public PrivateRequest {
   public:
    explicit OrderRequest(const std::string &name,
                          const std::string &params,
                          const Settings &settings,
                          NonceStorage::TakenValue &&,
                          const Context &context,
                          ModuleEventsLog &,
                          ModuleTradingLog &);
  };

 public:
  explicit CexioTradingSystem(const App &,
                              const TradingMode &,
                              Context &,
                              std::string instanceName,
                              std::string title,
                              const boost::property_tree::ptree &);
  ~CexioTradingSystem() override = default;

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
  void UpdateOrder(const OrderId &, const boost::property_tree::ptree &);

  boost::posix_time::ptime ParseTimeStamp(
      const std::string &key, const boost::property_tree::ptree &) const;

  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;
  mutable NonceStorage m_nonces;
  boost::unordered_map<std::string, CexioProduct> m_products;

  TradingLib::BalancesContainer m_balances;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPSClientSession> m_pollingSession;

  PollingTask m_pollingTask;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
