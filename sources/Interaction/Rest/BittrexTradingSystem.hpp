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

#include "BittrexAccount.hpp"
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
  using Settings = BittrexSettings;

  class OrderTransactionRequest;

  class AccountRequest : public BittrexPrivateRequest {
   public:
    typedef BittrexPrivateRequest Base;

    explicit AccountRequest(const std::string &name,
                            const std::string &uriParams,
                            const Settings &settings,
                            const Context &context,
                            ModuleEventsLog &log,
                            TradingLog *tradingLog = nullptr)
        : Base(name, uriParams, settings, context, log, tradingLog) {}
    ~AccountRequest() override = default;

   protected:
    bool IsPriority() const override { return false; }
  };

  class BalancesRequest : public AccountRequest {
   public:
    typedef AccountRequest Base;

    explicit BalancesRequest(const Settings &settings,
                             const Context &context,
                             ModuleEventsLog &log)
        : Base("/account/getbalances", std::string(), settings, context, log) {}
  };

 public:
  explicit BittrexTradingSystem(const App &,
                                const TradingMode &,
                                Context &,
                                std::string instanceName,
                                std::string title,
                                const boost::property_tree::ptree &);
  ~BittrexTradingSystem() override = default;

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

  Account &GetAccount() override;
  const Account &GetAccount() const override;

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

  boost::posix_time::ptime ParseTime(std::string &&) const;

  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  boost::unordered_map<std::string, BittrexProduct> m_products;

  TradingLib::BalancesContainer m_balances;
  BalancesRequest m_balancesRequest;

  BittrexAccount m_account;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPSClientSession> m_pollingSession;

  PollingTask m_pollingTask;
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
