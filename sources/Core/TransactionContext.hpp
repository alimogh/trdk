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

class TRDK_CORE_API OrderTransactionContext {
 public:
  explicit OrderTransactionContext(const trdk::OrderId &&orderId)
      : m_orderId(std::move(orderId)) {}
  virtual ~OrderTransactionContext() = default;

 public:
  const trdk::OrderId &GetOrderId() const { return m_orderId; }

 public:
  const trdk::OrderId m_orderId;
};
}
