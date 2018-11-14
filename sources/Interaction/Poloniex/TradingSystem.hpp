//
//    Created: 2018/11/14 15:24
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#pragma once

#include "Product.hpp"

namespace trdk {
namespace Interaction {
namespace Poloniex {

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
  TradingLib::BalancesContainer m_balances;
  boost::unordered_map<std::string, Product> m_products;
};

}  // namespace Poloniex
}  // namespace Interaction
}  // namespace trdk
