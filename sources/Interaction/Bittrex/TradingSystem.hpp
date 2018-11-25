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

#include "Request.hpp"
#include "Util.hpp"

namespace trdk {
namespace Interaction {
namespace Bittrex {

class TradingSystem : public trdk::TradingSystem {
 public:
  typedef trdk::TradingSystem Base;

 private:
  struct Settings : Rest::Settings {
    typedef Rest::Settings Base;

    std::string apiKey;
    std::string apiSecret;

    explicit Settings(const boost::property_tree::ptree &, ModuleEventsLog &);
  };

  class OrderTransactionRequest;

  class PrivateRequest : public Request {
   public:
    typedef Request Base;

    explicit PrivateRequest(const std::string &name,
                            const std::string &uriParams,
                            const Settings &settings,
                            const Context &context,
                            ModuleEventsLog &log,
                            ModuleTradingLog *tradingLog = nullptr);
    ~PrivateRequest() override = default;

   protected:
    void PrepareRequest(const Poco::Net::HTTPClientSession &,
                        const std::string &,
                        Poco::Net::HTTPRequest &) const override;
    void WriteUri(std::string uri, Poco::Net::HTTPRequest &) const override;

   private:
    const Settings &m_settings;
  };

  class AccountRequest : public PrivateRequest {
   public:
    typedef PrivateRequest Base;

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
  explicit TradingSystem(const Rest::App &,
                         const TradingMode &,
                         Context &,
                         std::string instanceName,
                         std::string title,
                         const boost::property_tree::ptree &);
  ~TradingSystem() override = default;

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

  bool AreWithdrawalSupported() const override;

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

  void SendWithdrawalTransaction(const std::string &,
                                 const Volume &,
                                 const std::string &) override;

  void OnTransactionSent(const OrderTransactionContext &) override;

 private:
  void UpdateBalances();
  void UpdateOrders();
  void UpdateOrder(const OrderId &, const boost::property_tree::ptree &);

  boost::posix_time::ptime ParseTime(std::string) const;

  Settings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  boost::unordered_map<std::string, Product> m_products;

  TradingLib::BalancesContainer m_balances;
  BalancesRequest m_balancesRequest;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPSClientSession> m_pollingSession;

  Rest::PollingTask m_pollingTask;
};

}  // namespace Bittrex
}  // namespace Interaction
}  // namespace trdk
