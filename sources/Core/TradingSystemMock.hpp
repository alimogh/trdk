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
  MOCK_METHOD1(Connect, void(const trdk::Lib::IniSectionRef &));

  MOCK_CONST_METHOD0(GetAccount, const trdk::TradingSystem::Account &());

  MOCK_METHOD10(SendOrder,
                boost::shared_ptr<const trdk::TransactionContext>(
                    trdk::Security &,
                    const trdk::Lib::Currency &,
                    const trdk::Qty &,
                    const boost::optional<trdk::Price> &,
                    const trdk::OrderParams &,
                    const trdk::TradingSystem::OrderStatusUpdateSlot &,
                    trdk::RiskControlScope &,
                    const trdk::OrderSide &,
                    const trdk::TimeInForce &,
                    const trdk::Lib::TimeMeasurement::Milestones &));

  MOCK_METHOD1(CancelOrder, void(const OrderId &));

  MOCK_METHOD1(OnSettingsUpdate, void(const trdk::Lib::IniSectionRef &));
  MOCK_METHOD1(CreateConnection, void(const trdk::Lib::IniSectionRef &));

  MOCK_METHOD7(SendOrderTransaction,
               std::unique_ptr<trdk::TransactionContext>(
                   trdk::Security &,
                   const trdk::Lib::Currency &,
                   const trdk::Qty &,
                   const boost::optional<trdk::Price> &,
                   const trdk::OrderParams &,
                   const trdk::OrderSide &,
                   const trdk::TimeInForce &));

  MOCK_METHOD1(SendCancelOrderTransaction, void(const OrderId &));
};
}
}
}