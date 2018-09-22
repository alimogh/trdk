//
//    Created: 2018/07/27 10:39 PM
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
namespace Huobi {

class TradingSystem : public trdk::TradingSystem {
 public:
  typedef trdk::TradingSystem Base;

 private:
  typedef boost::mutex CancelOrderMutex;
  typedef CancelOrderMutex::scoped_lock CancelOrderLock;

 public:
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

  void SendWithdrawalTransaction(const std::string &,
                                 const Volume &,
                                 const std::string &) override;

  void OnTransactionSent(const OrderTransactionContext &) override;

 private:
  void UpdateBalances();

  void SubscribeToOrderUpdates();
  bool UpdateOrders();
  void UpdateOrder(const OrderId &);

  AuthSettings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;
  boost::unordered_map<std::string, Product> m_products;

  PrivateRequest m_balancesRequest;
  TradingLib::BalancesContainer m_balances;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_tradingSession;
  std::unique_ptr<Poco::Net::HTTPSClientSession> m_pollingSession;

  Rest::PollingTask m_pollingTask;
};
}  // namespace Huobi
}  // namespace Interaction
}  // namespace trdk
