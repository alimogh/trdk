/*******************************************************************************
 *   Created: 2017/07/30 18:08:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "OrderStatusHandler.hpp"
#include "RiskControl.hpp"
#include "Security.hpp"
#include "TradingSystem.hpp"
#include "TransactionContext.hpp"

namespace trdk {
namespace Tests {
namespace Mocks {

class TradingSystem : public trdk::TradingSystem {
 public:
  TradingSystem();
  virtual ~TradingSystem() override = default;

  MOCK_CONST_METHOD0(GetMode, const trdk::TradingMode &());

  MOCK_CONST_METHOD0(GetIndex, size_t());

  MOCK_METHOD0(GetContext, trdk::Context &());
  MOCK_CONST_METHOD0(GetContext, const trdk::Context &());

  MOCK_CONST_METHOD0(GetInstanceName, const std::string &());

  MOCK_CONST_METHOD0(IsConnected, virtual bool());
  MOCK_METHOD0(Connect, void());

  virtual boost::shared_ptr<const trdk::OrderTransactionContext> SendOrder(
      trdk::Security &security,
      const trdk::Lib::Currency &currency,
      const trdk::Qty &qty,
      const boost::optional<trdk::Price> &price,
      const trdk::OrderParams &params,
      std::unique_ptr<trdk::OrderStatusHandler> &&handler,
      trdk::RiskControlScope &riskControlScope,
      const trdk::OrderSide &side,
      const trdk::TimeInForce &tif,
      const trdk::Lib::TimeMeasurement::Milestones &delayMeasurement) {
    return SendOrder(security, currency, qty, price, params, *handler,
                     riskControlScope, side, tif, delayMeasurement);
  }
  MOCK_METHOD10(SendOrder,
                boost::shared_ptr<const trdk::OrderTransactionContext>(
                    trdk::Security &,
                    const trdk::Lib::Currency &,
                    const trdk::Qty &,
                    const boost::optional<trdk::Price> &,
                    const trdk::OrderParams &,
                    trdk::OrderStatusHandler &,
                    trdk::RiskControlScope &,
                    const trdk::OrderSide &,
                    const trdk::TimeInForce &,
                    const trdk::Lib::TimeMeasurement::Milestones &));

  MOCK_METHOD1(CancelOrder, bool(const OrderId &));

  MOCK_METHOD0(CreateConnection, void());

  MOCK_METHOD7(SendOrderTransaction,
               std::unique_ptr<trdk::OrderTransactionContext>(
                   trdk::Security &,
                   const trdk::Lib::Currency &,
                   const trdk::Qty &,
                   const boost::optional<trdk::Price> &,
                   const trdk::OrderParams &,
                   const trdk::OrderSide &,
                   const trdk::TimeInForce &));

  MOCK_METHOD1(SendCancelOrderTransaction, void(const OrderId &));
};
}  // namespace Mocks
}  // namespace Tests
}  // namespace trdk