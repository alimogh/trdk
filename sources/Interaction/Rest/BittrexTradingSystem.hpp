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
#include "PullingTask.hpp"
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
                            const Settings &settings);
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
                            const Settings &settings)
        : Base(name, uriParams, settings) {}
    virtual ~AccountRequest() override = default;

   protected:
    virtual bool IsPriority() const override { return false; }
  };

  class BalancesRequest : public AccountRequest {
   public:
    typedef AccountRequest Base;

   public:
    explicit BalancesRequest(const Settings &settings)
        : Base("/account/getbalances", std::string(), settings) {}
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

  virtual void SendCancelOrderTransaction(const trdk::OrderId &) override;

  virtual void OnTransactionSent(const trdk::OrderId &) override;

 private:
  void UpdateBalances();
  void UpdateOrders();
  void UpdateOrder(const OrderId &, const boost::property_tree::ptree &);

 private:
  Settings m_settings;
  boost::unordered_map<std::string, BittrexProduct> m_products;

  BalancesContainer m_balances;
  BalancesRequest m_balancesRequest;

  Poco::Net::HTTPSClientSession m_tradingSession;
  Poco::Net::HTTPSClientSession m_pullingSession;

  PullingTask m_pullingTask;
};
}
}
}
