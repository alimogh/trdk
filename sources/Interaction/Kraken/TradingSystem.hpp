//
//    Created: 2018/10/14 8:51 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "AuthSettings.hpp"
#include "Product.hpp"
#include "Request.hpp"

namespace trdk {
namespace Interaction {
namespace Kraken {

class TradingSystem : public trdk::TradingSystem {
 public:
  typedef trdk::TradingSystem Base;

  explicit TradingSystem(const Rest::App &,
                         const TradingMode &,
                         Context &,
                         std::string instanceName,
                         std::string title,
                         const boost::property_tree::ptree &);
  TradingSystem(TradingSystem &&) = delete;
  TradingSystem(const TradingSystem &) = delete;
  TradingSystem &operator=(TradingSystem &&) = delete;
  TradingSystem &operator=(const TradingSystem &) = delete;
  ~TradingSystem() override = default;

  bool IsConnected() const override;

  Balances &GetBalancesStorage() override;

  Volume CalcCommission(const Qty &,
                        const Price &,
                        const OrderSide &,
                        const Security &) const override;

  boost::optional<OrderCheckError> CheckOrder(
      const Security &security,
      const Lib::Currency &currency,
      const Qty &qty,
      const boost::optional<Price> &price,
      const OrderSide &side) const override;

  bool CheckSymbol(const std::string &) const override;

  bool AreWithdrawalSupported() const override;

 protected:
  void CreateConnection() override;

  std::unique_ptr<OrderTransactionContext> SendOrderTransaction(
      Security &,
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

  void SubscribeToOrderUpdates();
  bool UpdateOrders();
  void UpdateOrder(const boost::posix_time::ptime &,
                   const OrderId &,
                   const boost::property_tree::ptree &);

  AuthSettings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;
  boost::unordered_map<std::string, Product> m_products;

  Rest::NonceStorage m_nonces;

  PrivateRequest m_balancesRequest;
  TradingLib::BalancesContainer m_balances;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPSClientSession> m_pollingSession;

  Rest::PollingTask m_pollingTask;
};
}  // namespace Kraken
}  // namespace Interaction
}  // namespace trdk
