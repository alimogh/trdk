/*******************************************************************************
 *   Created: 2017/11/03 00:42:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace FixProtocol {

class OrderTransactionContext : public trdk::OrderTransactionContext {
 public:
  typedef trdk::OrderTransactionContext Base;

 public:
  explicit OrderTransactionContext(const MessageSequenceNumber &orderId)
      : Base(orderId) {}
  virtual ~OrderTransactionContext() = default;

 public:
  void SetPositionId(const std::string &&positionId) {
    Assert(!positionId.empty());
    if (!m_positionId.empty()) {
      return;
    }
    m_positionId = std::move(positionId);
  }
  const std::string &GetPositionId() const { return m_positionId; }

 private:
  std::string m_positionId;
};
}
}
}
