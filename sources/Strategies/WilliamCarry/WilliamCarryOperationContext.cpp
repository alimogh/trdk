/*******************************************************************************
 *   Created: 2017/10/14 00:10:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "WilliamCarryOperationContext.hpp"
#include "TradingLib/OrderPolicy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;
using namespace trdk::Strategies::WilliamCarry;

class OperationContext::Implementation : private boost::noncopyable {
 public:
  const bool m_isLong;
  const Qty &m_qty;
  LimitIocOrderPolicy m_orderPolicy;

  explicit Implementation(bool isLong, const Qty &qty)
      : m_isLong(isLong), m_qty(qty) {}
};

OperationContext::OperationContext(bool isLong, const Qty &qty)
    : m_pimpl(boost::make_unique<Implementation>(isLong, qty)) {}

OperationContext::OperationContext(OperationContext &&) = default;

OperationContext::~OperationContext() = default;

const OrderPolicy &OperationContext::GetOpenOrderPolicy() const {
  return m_pimpl->m_orderPolicy;
}

const OrderPolicy &OperationContext::GetCloseOrderPolicy() const {
  return m_pimpl->m_orderPolicy;
}

void OperationContext::Setup(Position &) const {}

bool OperationContext::IsLong() const { return m_pimpl->m_isLong; }

Qty OperationContext::GetPlannedQty() const { return m_pimpl->m_qty; }

bool OperationContext::HasCloseSignal(const Position &) const { return false; }

bool OperationContext::IsInvertible(const Position &) const { return false; }
