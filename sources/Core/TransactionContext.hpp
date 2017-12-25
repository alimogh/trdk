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

#include "Api.h"

namespace trdk {

class OrderTransactionContext : private boost::noncopyable {
 public:
  explicit OrderTransactionContext(trdk::TradingSystem &tradingSystem,
                                   const trdk::OrderId &&orderId)
      : m_tradingSystem(tradingSystem), m_orderId(std::move(orderId)) {}
  virtual ~OrderTransactionContext() = default;

 public:
  trdk::TradingSystem &GetTradingSystem() { return m_tradingSystem; }
  const trdk::TradingSystem &GetTradingSystem() const {
    return const_cast<OrderTransactionContext *>(this)->GetTradingSystem();
  }

  const trdk::OrderId &GetOrderId() const { return m_orderId; }

 public:
  trdk::TradingSystem &m_tradingSystem;
  const trdk::OrderId m_orderId;
};
}
