/*******************************************************************************
 *   Created: 2017/10/01 04:14:20
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Client.hpp"
#include "Handler.hpp"

namespace trdk {
namespace Interaction {
namespace FixProtocol {

class TradingSystem : public trdk::TradingSystem, public Handler {
 public:
  typedef trdk::TradingSystem Base;

  explicit TradingSystem(const TradingMode&,
                         Context&,
                         std::string instanceName,
                         std::string title,
                         const boost::property_tree::ptree&);
  ~TradingSystem() override;

  Context& GetContext() override;
  const Context& GetContext() const override;

  ModuleEventsLog& GetLog() const override;

  bool IsConnected() const override;

  Volume CalcCommission(const Qty&,
                        const Price&,
                        const OrderSide&,
                        const trdk::Security&) const override;

  void OnConnectionRestored() override;

  void OnReject(const Incoming::Reject&, Lib::NetworkStreamClient&) override;
  void OnBusinessMessageReject(
      const Incoming::BusinessMessageReject&,
      Lib::NetworkStreamClient&,
      const Lib::TimeMeasurement::Milestones&) override;
  void OnExecutionReport(const Incoming::ExecutionReport&,
                         Lib::NetworkStreamClient&,
                         const Lib::TimeMeasurement::Milestones&) override;
  void OnOrderCancelReject(const Incoming::OrderCancelReject&,
                           Lib::NetworkStreamClient&,
                           const Lib::TimeMeasurement::Milestones&) override;

 protected:
  void CreateConnection() override;

  std::unique_ptr<OrderTransactionContext> SendOrderTransaction(
      trdk::Security& security,
      const Lib::Currency& currency,
      const Qty& qty,
      const boost::optional<Price>& price,
      const OrderParams& params,
      const OrderSide& side,
      const TimeInForce& tif) override;

  void SendCancelOrderTransaction(const OrderTransactionContext&) override;

 private:
  Client m_client;
};
}  // namespace FixProtocol
}  // namespace Interaction
}  // namespace trdk
