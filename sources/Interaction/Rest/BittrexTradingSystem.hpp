/*******************************************************************************
 *   Created: 2017/11/18 13:15:10
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "BittrexRequest.hpp"
#include "BittrexUtil.hpp"
#include "PollingTask.hpp"
#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

class BittrexTradingSystem : public TradingSystem {
 public:
  typedef TradingSystem Base;

 private:
  struct Settings : public Rest::Settings {
    typedef Rest::Settings Base;

    std::string apiKey;
    std::string apiSecret;

    explicit Settings(const Lib::IniSectionRef &, ModuleEventsLog &);
  };

  class OrderTransactionRequest;

  class PrivateRequest : public BittrexRequest {
   public:
    typedef BittrexRequest Base;

   public:
    explicit PrivateRequest(const std::string &name,
                            const std::string &uriParams,
                            const Settings &settings,
                            const Context &context,
                            ModuleEventsLog &log,
                            ModuleTradingLog *tradingLog = nullptr);
    virtual ~PrivateRequest() override = default;

   protected:
    virtual void PrepareRequest(const Poco::Net::HTTPClientSession &,
                                const std::string &,
                                Poco::Net::HTTPRequest &) const override;

   private:
    const Settings &m_settings;
  };

  class AccountRequest : public PrivateRequest {
   public:
    typedef PrivateRequest Base;

   public:
    explicit AccountRequest(const std::string &name,
                            const std::string &uriParams,
                            const Settings &settings,
                            const Context &context,
                            ModuleEventsLog &log,
                            TradingLog *tradingLog = nullptr)
        : Base(name, uriParams, settings, context, log, tradingLog) {}
    virtual ~AccountRequest() override = default;

   protected:
    virtual bool IsPriority() const override { return false; }
  };

  class BalancesRequest : public AccountRequest {
   public:
    typedef AccountRequest Base;

   public:
    explicit BalancesRequest(const Settings &settings,
                             const Context &context,
                             ModuleEventsLog &log)
        : Base("/account/getbalances", std::string(), settings, context, log) {}
  };

 public:
  explicit BittrexTradingSystem(const App &,
                                const TradingMode &,
                                Context &,
                                const std::string &instanceName,
                                const Lib::IniSectionRef &);
  virtual ~BittrexTradingSystem() override = default;

 public:
  virtual bool IsConnected() const override { return !m_products.empty(); }

  virtual Balances &GetBalancesStorage() override { return m_balances; }

  virtual Volume CalcCommission(const Volume &vol,
                                const trdk::Security &security) const override {
    return RoundByPrecision(vol * (0.25 / 100),
                            security.GetPricePrecisionPower());
  }

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

  boost::posix_time::ptime ParseTime(std::string &&) const;

 private:
  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  boost::unordered_map<std::string, BittrexProduct> m_products;

  BalancesContainer m_balances;
  BalancesRequest m_balancesRequest;

  std::unique_ptr<Poco::Net::HTTPClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPClientSession> m_pollingSession;

  PollingTask m_pollingTask;
};
}
}
}
