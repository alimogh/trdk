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

  explicit OrderTransactionContext(TradingSystem &tradingSystem,
                                   const MessageSequenceNumber &orderId)
      : Base(tradingSystem, orderId) {}
  OrderTransactionContext(OrderTransactionContext &&) = default;
  OrderTransactionContext(const OrderTransactionContext &) = default;
  OrderTransactionContext &operator=(OrderTransactionContext &&) = default;
  OrderTransactionContext &operator=(const OrderTransactionContext &) = default;
  ~OrderTransactionContext() = default;

  void SetPositionId(std::string positionId) {
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
}  // namespace FixProtocol
}  // namespace Interaction
}  // namespace trdk
