/*******************************************************************************
 *   Created: 2017/10/13 10:42:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Operation.hpp"
#include "Context.hpp"
#include "MarketDataSource.hpp"
#include "Security.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace trdk::Lib;

namespace uuids = boost::uuids;

namespace {
thread_local uuids::random_generator generateUuid;
}

class Operation::Implementation : private boost::noncopyable {
 public:
  uuids::uuid m_id;

  Implementation() : m_id(generateUuid()) {}
};

Operation::Operation() : m_pimpl(boost::make_unique<Implementation>()) {}

Operation::Operation(Operation &&) = default;

Operation::~Operation() = default;

const uuids::uuid &Operation::GetId() const { return m_pimpl->m_id; }

TradingSystem &Operation::GetTradingSystem(Strategy &strategy,
                                           Security &security) {
  return strategy.GetTradingSystem(security.GetSource().GetIndex());
}
const TradingSystem &Operation::GetTradingSystem(
    const Strategy &strategy, const Security &security) const {
  return strategy.GetTradingSystem(security.GetSource().GetIndex());
}

bool Operation::OnCloseReasonChange(Position &, const CloseReason &) {
  return true;
}

const Operation *Operation::GetParent() const { return nullptr; }
Operation *Operation::GetParent() { return nullptr; }

bool Operation::HasCloseSignal(const Position &) const {
  throw LogicError(
      "Position instance does not use operation context to detect signal to "
      "close");
}

boost::shared_ptr<Operation> Operation::StartInvertedPosition(
    const Position &) {
  throw LogicError(
      "Position instance does not use operation context to invert position");
}
