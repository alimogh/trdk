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
                       const trdk::ScaledPrice &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));
  MOCK_METHOD8(SellAtMarketPriceWithStopPrice,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::ScaledPrice &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));
  MOCK_METHOD8(SellImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::ScaledPrice &,
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
                       const trdk::ScaledPrice &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));
  MOCK_METHOD8(BuyAtMarketPriceWithStopPrice,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::ScaledPrice &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &,
                       trdk::RiskControlScope &,
                       const trdk::Lib::TimeMeasurement::Milestones &));
  MOCK_METHOD8(BuyImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::ScaledPrice &,
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

  MOCK_METHOD5(SendSellAtMarketPrice,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &));
  MOCK_METHOD6(SendSell,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::ScaledPrice &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &));
  virtual OrderId SendSell(trdk::Security &arg1,
                           const trdk::Lib::Currency &arg2,
                           const trdk::Qty &arg3,
                           const trdk::ScaledPrice &arg4,
                           const trdk::OrderParams &arg5,
                           const OrderStatusUpdateSlot &&arg6) override {
    //! @todo Remove this method when Google Test will support movement
    //! semantics.
    return SendSell(arg1, arg2, arg3, arg4, arg5, arg6);
  }
  MOCK_METHOD6(SendSellAtMarketPriceWithStopPrice,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::ScaledPrice &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &));
  MOCK_METHOD6(SendSellImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::ScaledPrice &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &));
  MOCK_METHOD5(SendSellAtMarketPriceImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &));

  MOCK_METHOD5(SendBuyAtMarketPrice,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &));
  MOCK_METHOD6(SendBuy,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::ScaledPrice &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &));
  virtual OrderId SendBuy(trdk::Security &arg1,
                          const trdk::Lib::Currency &arg2,
                          const trdk::Qty &arg3,
                          const trdk::ScaledPrice &arg4,
                          const trdk::OrderParams &arg5,
                          const OrderStatusUpdateSlot &&arg6) override {
    //! @todo Remove this method when Google Test will support movement
    //! semantics.
    return SendBuy(arg1, arg2, arg3, arg4, arg5, arg6);
  }
  MOCK_METHOD6(SendBuyAtMarketPriceWithStopPrice,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::ScaledPrice &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &));
  MOCK_METHOD6(SendBuyImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::ScaledPrice &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &));
  MOCK_METHOD5(SendBuyAtMarketPriceImmediatelyOrCancel,
               OrderId(trdk::Security &,
                       const trdk::Lib::Currency &,
                       const trdk::Qty &,
                       const trdk::OrderParams &,
                       const OrderStatusUpdateSlot &));

  MOCK_METHOD1(SendCancelOrder, void(const OrderId &));
};
}
}
}