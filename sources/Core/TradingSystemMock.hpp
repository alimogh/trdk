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

  MOCK_METHOD7(SellAtMarketPrice,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));
  MOCK_METHOD8(Sell,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::Price &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));
  MOCK_METHOD8(SellImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::Price &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));
  MOCK_METHOD7(SellAtMarketPriceImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));

  MOCK_METHOD7(BuyAtMarketPrice,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));
  MOCK_METHOD8(Buy,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::Price &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));
  MOCK_METHOD8(BuyImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::Price &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));
  MOCK_METHOD7(BuyAtMarketPriceImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));

  MOCK_METHOD1(CancelOrder, void(const OrderId &));

  MOCK_METHOD0(Test, void());

  MOCK_METHOD1(OnSettingsUpdate, void(const trdk::Lib::IniSectionRef &));
  MOCK_METHOD1(CreateConnection, void(const trdk::Lib::IniSectionRef &));

  MOCK_METHOD4(SendSellAtMarketPrice,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &));
  MOCK_METHOD5(SendSell,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::Price &,
                       const trdk::OrderParams &));
  MOCK_METHOD5(SendSellImmediatelyOrCancel,
               trdk::OrderId(trdk::Security &,
                             const trdk::Lib::Currency &,
                             const trdk::Qty &,
                             const trdk::Price &,
                             const trdk::OrderParams &));
  MOCK_METHOD4(SendSellAtMarketPriceImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &));
  MOCK_METHOD4(SendBuyAtMarketPrice,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &));
  MOCK_METHOD5(SendBuy,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::Price &,
                       const trdk::OrderParams &));
  MOCK_METHOD5(SendBuyImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::Price &,
                       const trdk::OrderParams &));
  MOCK_METHOD4(SendBuyAtMarketPriceImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &));

  MOCK_METHOD1(SendCancelOrder, void(const OrderId &));
};
}
}
}