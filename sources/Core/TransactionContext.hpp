/*******************************************************************************
 *   Created: 2017/11/02 23:26:06
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {

class OrderTransactionContext {
 public:
  explicit OrderTransactionContext(TradingSystem& tradingSystem,
                                   OrderId orderId)
      : m_tradingSystem(tradingSystem), m_orderId(std::move(orderId)) {}
  OrderTransactionContext(OrderTransactionContext&&) = default;
  OrderTransactionContext(const OrderTransactionContext&) = default;
  OrderTransactionContext& operator=(OrderTransactionContext&&) = default;
  OrderTransactionContext& operator=(const OrderTransactionContext&) = default;
  virtual ~OrderTransactionContext() = default;

  TradingSystem& GetTradingSystem() { return m_tradingSystem; }

  const TradingSystem& GetTradingSystem() const {
    return const_cast<OrderTransactionContext*>(this)->GetTradingSystem();
  }

  const OrderId& GetOrderId() const { return m_orderId; }

  TradingSystem& m_tradingSystem;
  const OrderId m_orderId;
};

}  // namespace trdk
