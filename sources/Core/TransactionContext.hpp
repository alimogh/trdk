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

class OrderTransactionContext : boost::noncopyable {
 public:
  explicit OrderTransactionContext(TradingSystem& tradingSystem,
                                   OrderId&& orderId)
      : m_tradingSystem(tradingSystem), m_orderId(std::move(orderId)) {}

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
