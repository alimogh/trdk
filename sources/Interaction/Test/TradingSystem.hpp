/**************************************************************************
 *   Created: 2012/07/23 23:13:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Context.hpp"
#include "Core/TradingSystem.hpp"

namespace trdk {
namespace Interaction {
namespace Test {

class TradingSystem : public trdk::TradingSystem {
 public:
  typedef trdk::TradingSystem Base;

  explicit TradingSystem(const TradingMode &,
                         Context &context,
                         const std::string &instanceName,
                         const boost::property_tree::ptree &);
  ~TradingSystem() override;

  bool IsConnected() const override;

  Volume CalcCommission(const Qty &,
                        const Price &,
                        const OrderSide &,
                        const Security &) const override;

 protected:
  std::unique_ptr<OrderTransactionContext> SendOrderTransaction(
      Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderParams &,
      const OrderSide &,
      const TimeInForce &) override;
  void SendCancelOrderTransaction(const OrderTransactionContext &) override;

 public:
  void CreateConnection() override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace Test
}  // namespace Interaction
}  // namespace trdk
