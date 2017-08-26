/*******************************************************************************
 *   Created: 2017/08/26 19:08:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PositionController.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/Position.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "PositionReport.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::TradingLib;

namespace ids = boost::uuids;

class PositionController::Implementation : private boost::noncopyable {
 public:
  PositionController &m_self;
  Strategy &m_strategy;
  ids::random_generator m_generateUuid;

  std::unique_ptr<PositionReport> m_report;

 public:
  explicit Implementation(PositionController &self, Strategy &strategy)
      : m_self(self), m_strategy(strategy) {}

  PositionReport &GetReport() {
    if (!m_report) {
      m_report = m_self.OpenReport();
      Assert(m_report);
    }
    return *m_report;
  }
};

PositionController::PositionController(Strategy &strategy)
    : m_pimpl(std::make_unique<Implementation>(*this, strategy)) {}

PositionController::~PositionController() {}

Strategy &PositionController::GetStrategy() { return m_pimpl->m_strategy; }
const Strategy &PositionController::GetStrategy() const {
  return const_cast<PositionController *>(this)->GetStrategy();
}

std::unique_ptr<PositionReport> PositionController::OpenReport() const {
  return std::make_unique<PositionReport>(GetStrategy());
}

ids::uuid PositionController::GenerateNewOperationId() const {
  return m_pimpl->m_generateUuid();
}
TradingSystem &PositionController::GetTradingSystem(const Security &security) {
  return GetStrategy().GetTradingSystem(security.GetSource().GetIndex());
}

trdk::Position &PositionController::OpenPosition(
    Security &security, bool isLong, const Milestones &delayMeasurement) {
  auto position =
      isLong ? CreatePosition<LongPosition>(
                   security, security.GetAskPriceScaled(), delayMeasurement)
             : CreatePosition<ShortPosition>(
                   security, security.GetBidPriceScaled(), delayMeasurement);
  ContinuePosition(*position);
  return *position;
}

void PositionController::OnSignal(Security &security,
                                  const Milestones &delayMeasurement) {
  Position *position = nullptr;
  if (!GetStrategy().GetPositions().IsEmpty()) {
    AssertEq(1, GetStrategy().GetPositions().GetSize());
    position = &*GetStrategy().GetPositions().GetBegin();
    if (position->IsCompleted()) {
      position = nullptr;
    } else if (IsPositionCorrect(*position)) {
      delayMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
      return;
    }
  }

  if (position &&
      (position->IsCancelling() || position->HasActiveCloseOrders())) {
    return;
  }

  delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_1);

  if (!position) {
    OpenPosition(security, delayMeasurement);
  } else if (position->HasActiveOpenOrders()) {
    try {
      Verify(position->CancelAllOrders());
    } catch (const TradingSystem::UnknownOrderCancelError &ex) {
      GetStrategy().GetTradingLog().Write("failed to cancel order");
      GetStrategy().GetLog().Warn("Failed to cancel order: \"%1%\".",
                                  ex.what());
      return;
    }
  } else {
    ClosePosition(*position, CLOSE_REASON_SIGNAL);
  }

  delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);
}

void PositionController::OnPositionUpdate(Position &position) {
  AssertLt(0, position.GetNumberOfOpenOrders());

  if (position.IsCompleted()) {
    // No active order, no active qty...

    Assert(!position.HasActiveOrders());
    if (position.GetNumberOfCloseOrders()) {
      // Position fully closed.
      m_pimpl->GetReport().Append(position);
      if (!IsPositionCorrect(position)) {
        // Opening opposite position.
        OpenPosition(position.GetSecurity(), !position.IsLong(), Milestones());
      }
      return;
    }
    // Open order was canceled by some condition. Checking open
    // signal again and sending new open order...
    if (IsPositionCorrect(position)) {
      ContinuePosition(position);
    }

    // Position will be deleted if was not continued.

  } else if (position.GetNumberOfCloseOrders()) {
    // Position closing started.

    Assert(!position.HasActiveOpenOrders());
    AssertNe(CLOSE_REASON_NONE, position.GetCloseReason());

    if (position.HasActiveCloseOrders()) {
      // Closing in progress.
    } else if (position.GetCloseReason() != CLOSE_REASON_NONE) {
      // Close order was canceled by some condition. Sending
      // new close order.
      ClosePosition(position, CLOSE_REASON_SIGNAL);
    }

  } else if (position.HasActiveOrders()) {
    // Opening in progress.

    Assert(!position.HasActiveCloseOrders());

    if (!IsPositionCorrect(position)) {
      // Close signal received, closing position...
      ClosePosition(position, CLOSE_REASON_SIGNAL);
    }

  } else {
    // Position is active.

    if (!IsPositionCorrect(position)) {
      // Received signal to close...
      AssertLt(0, position.GetActiveQty());
      ClosePosition(position, CLOSE_REASON_SIGNAL);
    }
  }
}
