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
    std::string apiKey;
    std::string apiSecret;

    explicit Settings(const Lib::IniSectionRef &, ModuleEventsLog &);
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

 private:
  Settings m_settings;
  boost::unordered_map<std::string, CexioProduct> m_products;

  BalancesContainer m_balances;
  // BalancesRequest m_balancesRequest;

  std::unique_ptr<Poco::Net::HTTPClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPClientSession> m_pollingSession;

  PollingTask m_pollingTask;
};
}
}
}
