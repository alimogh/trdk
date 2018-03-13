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
#include "DropCopy.hpp"
#include "MarketDataSource.hpp"
#include "PnlContainer.hpp"
#include "Security.hpp"
#include "Strategy.hpp"
#include "TradingLog.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::TradingLib;

namespace uuids = boost::uuids;

namespace {
thread_local uuids::random_generator generateUuid;
}

class Operation::Implementation : private boost::noncopyable {
 public:
  Strategy &m_strategy;
  uuids::uuid m_id;

  boost::mutex m_startMutex;
  bool m_isStarted;

  std::unique_ptr<PnlContainer> m_pnl;

  explicit Implementation(Strategy &strategy,
                          std::unique_ptr<PnlContainer> &&pnl)
      : m_strategy(strategy),
        m_id(generateUuid()),
        m_isStarted(false),
        m_pnl(std::move(pnl)) {}

  ~Implementation() {
    try {
      if (m_isStarted) {
        m_strategy.GetTradingLog().Write(
            "{'operation': {'result:': %1%, 'finResult': {%2%}, 'id' : '%3%'}}",
            [this](TradingRecord &record) {
              record % m_pnl->GetResult();  // 1
              std::string list;
              for (const auto &pnl : m_pnl->GetData()) {
                if (!list.empty()) {
                  list += ", ";
                }
                list += "'" + pnl.first +
                        "': " + boost::lexical_cast<std::string>(pnl.second);
              }
              record % list  // 2
                  % m_id;    // 3
            });
        m_strategy.GetContext().InvokeDropCopy([this](DropCopy &dropCopy) {
          dropCopy.CopyOperationEnd(m_id,
                                    m_strategy.GetContext().GetCurrentTime(),
                                    std::unique_ptr<Pnl>(m_pnl.release()));
        });
      }
    } catch (...) {
      AssertFailNoException();
    }
  }
};

Operation::Operation(Strategy &strategy, std::unique_ptr<PnlContainer> &&pnl)
    : m_pimpl(boost::make_unique<Implementation>(strategy, std::move(pnl))) {}

Operation::Operation(Operation &&) = default;

Operation::~Operation() = default;

const uuids::uuid &Operation::GetId() const { return m_pimpl->m_id; }

const Strategy &Operation::GetStrategy() const { return m_pimpl->m_strategy; }
Strategy &Operation::GetStrategy() { return m_pimpl->m_strategy; }

TradingSystem &Operation::GetTradingSystem(Security &security) {
  return GetStrategy().GetTradingSystem(security.GetSource().GetIndex());
}
const TradingSystem &Operation::GetTradingSystem(
    const Security &security) const {
  return GetStrategy().GetTradingSystem(security.GetSource().GetIndex());
}

const OrderPolicy &Operation::GetOpenOrderPolicy(const Position &) const {
  Assert(m_pimpl->m_isStarted);
  throw LogicError(
      "Position instance does not use operation context to open positions");
}

const OrderPolicy &Operation::GetCloseOrderPolicy(const Position &) const {
  Assert(m_pimpl->m_isStarted);
  throw LogicError(
      "Position instance does not use operation context to close positions");
}

void Operation::Setup(Position &, PositionController &) const {
  Assert(m_pimpl->m_isStarted);
}

bool Operation::IsLong(const Security &) const {
  throw LogicError(
      "Position instance does not use operation context to get order side");
}

Qty Operation::GetPlannedQty(const Security &) const {
  throw LogicError(
      "Position instance does not use operation context to calculate position "
      "planned size");
}

bool Operation::OnCloseReasonChange(Position &, const CloseReason &) {
  Assert(m_pimpl->m_isStarted);
  return true;
}

boost::shared_ptr<const Operation> Operation::GetParent() const {
  return nullptr;
}
boost::shared_ptr<Operation> Operation::GetParent() { return nullptr; }

bool Operation::HasCloseSignal(const Position &) const {
  Assert(m_pimpl->m_isStarted);
  throw LogicError(
      "Position instance does not use operation context to detect signal to "
      "close");
}

boost::shared_ptr<Operation> Operation::StartInvertedPosition(
    const Position &) {
  Assert(m_pimpl->m_isStarted);
  throw LogicError(
      "Position instance does not use operation context to invert position");
}

void Operation::UpdatePnl(const Security &security,
                          const OrderSide &side,
                          const Qty &qty,
                          const Price &price,
                          const Volume &comission) {
  Assert(m_pimpl->m_isStarted);
  if (m_pimpl->m_pnl->Update(security, side, qty, price, comission)) {
    GetStrategy().GetContext().InvokeDropCopy([this](DropCopy &dropCopy) {
      dropCopy.CopyOperationUpdate(GetId(), m_pimpl->m_pnl->GetData());
    });
  }
}

const Pnl &Operation::GetPnl() const { return *m_pimpl->m_pnl; }

void Operation::OnNewPositionStart(Position &) {
  // As positions may be created in parallel threads for one strategy - method
  // is not thread-safe.
  const boost::mutex::scoped_lock lock(m_pimpl->m_startMutex);
  if (m_pimpl->m_isStarted) {
    return;
  }
  m_pimpl->m_isStarted = true;
  GetStrategy().GetContext().InvokeDropCopy([this](DropCopy &dropCopy) {
    dropCopy.CopyOperationStart(
        GetId(), GetStrategy().GetContext().GetCurrentTime(), GetStrategy());
  });
}
