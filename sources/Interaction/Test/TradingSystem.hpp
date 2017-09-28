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
                         size_t index,
                         Context &context,
                         const std::string &instanceName,
                         const Lib::IniSectionRef &);
  virtual ~TradingSystem();

 public:
  virtual bool IsConnected() const override;

 protected:
  virtual OrderId SendSellAtMarketPrice(trdk::Security &,
                                        const trdk::Lib::Currency &,
                                        const trdk::Qty &,
                                        const trdk::OrderParams &,
                                        const OrderStatusUpdateSlot &) override;
  virtual OrderId SendSell(trdk::Security &,
                           const trdk::Lib::Currency &,
                           const trdk::Qty &,
                           const trdk::Price &,
                           const trdk::OrderParams &,
                           const OrderStatusUpdateSlot &&) override;
  virtual OrderId SendSellAtMarketPriceWithStopPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &stopPrice,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;
  virtual OrderId SendSellImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;
  virtual OrderId SendSellAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;

  virtual OrderId SendBuyAtMarketPrice(trdk::Security &,
                                       const trdk::Lib::Currency &,
                                       const trdk::Qty &,
                                       const trdk::OrderParams &,
                                       const OrderStatusUpdateSlot &) override;
  virtual OrderId SendBuy(trdk::Security &,
                          const trdk::Lib::Currency &,
                          const trdk::Qty &,
                          const trdk::Price &,
                          const trdk::OrderParams &,
                          const OrderStatusUpdateSlot &&) override;
  virtual OrderId SendBuyAtMarketPriceWithStopPrice(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &stopPrice,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;
  virtual OrderId SendBuyImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::Price &,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;
  virtual OrderId SendBuyAtMarketPriceImmediatelyOrCancel(
      trdk::Security &,
      const trdk::Lib::Currency &,
      const trdk::Qty &,
      const trdk::OrderParams &,
      const OrderStatusUpdateSlot &) override;

  virtual void SendCancelOrder(const OrderId &) override;

 public:
  virtual void CreateConnection(const Lib::IniSectionRef &) override;

 public:
  virtual void OnSettingsUpdate(const Lib::IniSectionRef &) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}
