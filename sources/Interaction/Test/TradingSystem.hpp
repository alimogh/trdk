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

 public:
  explicit TradingSystem(const trdk::TradingMode &,
                         Context &context,
                         const std::string &instanceName,
                         const Lib::IniSectionRef &);
  virtual ~TradingSystem() override;

 public:
  virtual bool IsConnected() const override;

  virtual trdk::Volume CalcCommission(const trdk::Qty &,
                                      const trdk::Price &,
                                      const trdk::OrderSide &,
                                      const trdk::Security &) const override;

 protected:
  virtual std::unique_ptr<OrderTransactionContext> SendOrderTransaction(
      Security &,
      const Lib::Currency &,
      const Qty &,
      const boost::optional<Price> &,
      const OrderParams &,
      const OrderSide &,
      const TimeInForce &) override;
  virtual void SendCancelOrderTransaction(
      const OrderTransactionContext &) override;

 public:
  virtual void CreateConnection(const Lib::IniSectionRef &) override;

 public:
  virtual void OnSettingsUpdate(const Lib::IniSectionRef &) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace Test
}  // namespace Interaction
}  // namespace trdk
