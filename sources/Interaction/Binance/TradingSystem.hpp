//
//    Created: 2018/04/07 3:47 PM
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

namespace trdk {
namespace Interaction {
namespace Binance {

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

  Volume CalcCommission(const Qty &,
                        const Price &,
                        const OrderSide &,
                        const trdk::Security &) const override;

  Balances &GetBalancesStorage() override;

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

  boost::optional<OrderCheckError> CheckOrder(
      const trdk::Security &security,
      const Lib::Currency &currency,
      const Qty &qty,
      const boost::optional<Price> &price,
      const OrderSide &side) const override;

  bool CheckSymbol(const std::string &) const override;

 private:
  boost::shared_ptr<TradingSystemConnection> CreateListeningConnection();
  void ScheduleListeningConnectionReconnect();

  void HandleMessage(const boost::property_tree::ptree &);

  void UpdateBalances(const boost::property_tree::ptree &);
  void UpdateOrder(const boost::property_tree::ptree &);

  AuthSettings m_settings;
  const boost::posix_time::time_duration m_serverTimeDiff;

  std::string m_key;
  const boost::unordered_map<std::string, Product> *m_products = nullptr;

  TradingLib::BalancesContainer m_balances;

  std::unique_ptr<Poco::Net::HTTPSClientSession> m_session;

  boost::mutex m_listeningConnectionMutex;
  boost::shared_ptr<TradingSystemConnection> m_listeningConnection;

  Timer::Scope m_timerScope;
};

}  // namespace Binance
}  // namespace Interaction
}  // namespace trdk
