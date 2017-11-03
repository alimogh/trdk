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

class TRDK_CORE_API TransactionContext {
 public:
  explicit TransactionContext(size_t id) : m_id(id) {}
  virtual ~TransactionContext() = default;

 public:
  //! Unique internal transaction ID.
  /** The same as "order ID", but, after refactoring (order ID will be string),
    * should not be used as order ID. This field should be renamed to "GetId"
    * and used only for log.
    */
  size_t GetOrderId() const { return m_id; }

 public:
  const size_t m_id;
};
}
