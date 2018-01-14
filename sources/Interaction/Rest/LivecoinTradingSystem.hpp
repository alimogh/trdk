/*******************************************************************************
 *   Created: 2017/12/19 21:00:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

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
  struct Settings : public Rest::Settings {
    std::string apiKey;
    std::string apiSecret;

    explicit Settings(const Lib::IniSectionRef &, ModuleEventsLog &);
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
    virtual ~PrivateRequest() override = default;

   protected:
    virtual void PrepareRequest(const Poco::Net::HTTPClientSession &,
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
    virtual ~TradingRequest() override = default;

   public:
    Response Send(Poco::Net::HTTPClientSession &) override;

   protected:
    virtual bool IsPriority() const override { return true; }
  };

  class BalancesRequest : public PrivateRequest {
   public:
    explicit BalancesRequest(const Settings &,
                             const Context &,
                             ModuleEventsLog &);
    virtual ~BalancesRequest() override = default;

   protected:
    virtual bool IsPriority() const override { return false; }
  };

 public:
  explicit LivecoinTradingSystem(const App &,
                                 const TradingMode &,
                                 Context &,
                                 const std::string &instanceName,
                                 const Lib::IniSectionRef &);
  virtual ~LivecoinTradingSystem() override = default;

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

 private:
  Settings m_settings;
  boost::unordered_map<std::string, LivecoinProduct> m_products;

  BalancesContainer m_balances;
  BalancesRequest m_balancesRequest;

  std::unique_ptr<Poco::Net::HTTPClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPClientSession> m_pollingSession;

  PollingTask m_pollingTask;
};
}
}
}