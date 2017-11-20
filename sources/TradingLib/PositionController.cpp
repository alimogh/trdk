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
#include "Core/PositionOperationContext.hpp"
#include "Core/Strategy.hpp"
#include "Core/TradingLog.hpp"
#include "OrderPolicy.hpp"
#include "PositionReport.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Lib::TimeMeasurement;
using namespace trdk::TradingLib;

namespace ids = boost::uuids;

class PositionController::Implementation : private boost::noncopyable {
 public:
  PositionController &m_self;
  ids::random_generator m_generateUuid;

  Strategy &m_strategy;

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

PositionController::~PositionController() = default;

Strategy &PositionController::GetStrategy() { return m_pimpl->m_strategy; }
const Strategy &PositionController::GetStrategy() const {
  return const_cast<PositionController *>(this)->GetStrategy();
}

std::unique_ptr<PositionReport> PositionController::OpenReport() const {
  return std::make_unique<PositionReport>(GetStrategy());
}

const PositionReport &PositionController::GetReport() const {
  return m_pimpl->GetReport();
}

ids::uuid PositionController::GenerateNewOperationId() const {
  return m_pimpl->m_generateUuid();
}

trdk::Position &PositionController::OpenPosition(
    const boost::shared_ptr<PositionOperationContext> &operationContext,
    Security &security,
    const Milestones &delayMeasurement) {
  return OpenPosition(operationContext, security,
                      operationContext->IsLong(security), delayMeasurement);
}

trdk::Position &PositionController::OpenPosition(
    const boost::shared_ptr<PositionOperationContext> &operationContext,
    Security &security,
    bool isLong,
    const Milestones &delayMeasurement) {
  return OpenPosition(operationContext, security, isLong,
                      operationContext->GetPlannedQty(), delayMeasurement);
}

trdk::Position &PositionController::OpenPosition(
    const boost::shared_ptr<PositionOperationContext> &operationContext,
    Security &security,
    bool isLong,
    const Qty &qty,
    const Milestones &delayMeasurement) {
  auto result =
      CreatePosition(operationContext, isLong, security, qty,
                     isLong ? security.GetAskPrice() : security.GetBidPrice(),
                     delayMeasurement);
  ContinuePosition(*result);
  return *result;
}

Position &PositionController::RestorePosition(
    const boost::shared_ptr<PositionOperationContext> &operationContext,
    Security &security,
    bool isLong,
    const Qty &qty,
    const Price &startPrice,
    const Price &openPrice,
    const boost::shared_ptr<const OrderTransactionContext> &openingContext,
    const Milestones &delayMeasurement) {
  auto result = CreatePosition(operationContext, isLong, security, qty,
                               startPrice, delayMeasurement);
  result->RestoreOpenState(openPrice, openingContext);
  return *result;
}

void PositionController::ContinuePosition(Position &position) {
  Assert(!position.HasActiveOrders());
  position.GetOperationContext().GetOpenOrderPolicy(position).Open(position);
}

void PositionController::HoldPosition(Position &) {}

bool PositionController::ClosePosition(Position &position,
                                       const CloseReason &reason) {
  IsPassive(reason) ? position.SetCloseReason(reason)
                    : position.ResetCloseReason(reason);
  if (position.HasActiveOpenOrders()) {
    if (position.IsCancelling()) {
      return false;
    }
    try {
      Verify(position.CancelAllOrders());
      return true;
    } catch (const TradingSystem::OrderIsUnknown &) {
      return false;
    }
  } else if (position.HasActiveCloseOrders()) {
    return false;
  } else {
    ClosePosition(position);
    return true;
  }
}

void PositionController::ClosePosition(Position &position) {
  Assert(!position.HasActiveOrders());
  position.GetOperationContext().GetCloseOrderPolicy(position).Close(position);
}

void PositionController::CloseAllPositions(const CloseReason &reason) {
  for (auto &position : GetStrategy().GetPositions()) {
    try {
      ClosePosition(position, reason);
    } catch (const std::exception &ex) {
      GetStrategy().GetLog().Error(
          "Failed to close position \"%1%\" with reason %2%: \"%2%\".",
          position.GetId(), reason, ex.what());
    }
  }
}

Position *PositionController::OnSignal(
    const boost::shared_ptr<PositionOperationContext> &newOperationContext,
    Security &security,
    const Milestones &delayMeasurement) {
  Position *position = nullptr;
  if (!GetStrategy().GetPositions().IsEmpty()) {
    AssertEq(1, GetStrategy().GetPositions().GetSize());
    position = &*GetStrategy().GetPositions().GetBegin();
    if (position->IsCompleted()) {
      position = nullptr;
    } else if (!position->GetOperationContext().HasCloseSignal(*position)) {
      if (!position->HasActiveCloseOrders()) {
        position->ResetCloseReason();
      }
      delayMeasurement.Measure(SM_STRATEGY_WITHOUT_DECISION_1);
      return nullptr;
    }
  }

  if (position &&
      (position->IsCancelling() || position->HasActiveCloseOrders())) {
    return nullptr;
  }

  delayMeasurement.Measure(SM_STRATEGY_EXECUTION_START_1);

  if (!position) {
    position = &OpenPosition(newOperationContext, security, delayMeasurement);
  } else if (!ClosePosition(*position, CLOSE_REASON_SIGNAL)) {
    return nullptr;
  }

  delayMeasurement.Measure(SM_STRATEGY_EXECUTION_COMPLETE_1);

  return position;
}

void PositionController::OnPositionUpdate(Position &position) {
  AssertLt(0, position.GetNumberOfOpenOrders());

  if (position.IsCompleted()) {
    if (position.GetOpenedQty() > 0) {
      // It seems position is marked as completed for some reason. It's not a
      // business of the general code.
      return;
    }

    // No active order, no active qty...

    Assert(!position.HasActiveOrders());
    if (position.GetNumberOfCloseOrders()) {
      // Position fully closed.
      m_pimpl->GetReport().Append(position);
      auto &context = position.GetOperationContext();
      if (context.HasCloseSignal(position)) {
        const auto &newOperationContext =
            context.StartInvertedPosition(position);
        if (newOperationContext) {
          //! @todo Move StartInvertedPosition to the closing start and close
          //! position x2 with "restoring" position object as opposite position.
          OpenPosition(std::move(newOperationContext), position.GetSecurity(),
                       !position.IsLong(), Milestones());
        }
      }
      return;
    }
    // Open order was canceled by some condition. Checking open
    // signal again and sending new open order...
    if (position.GetCloseReason() == CLOSE_REASON_NONE &&
        !position.GetOperationContext().HasCloseSignal(position)) {
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
      ClosePosition(position);
    }

  } else if (position.HasActiveOrders()) {
    // Opening in progress.

    Assert(!position.HasActiveCloseOrders());

    if (position.GetCloseReason() != CLOSE_REASON_NONE) {
      // Close signal received, closing position...
      ClosePosition(position);
    }

  } else {
    // Position is active.

    if (position.GetCloseReason() != CLOSE_REASON_NONE) {
      // Received signal to close...
      AssertLt(0, position.GetActiveQty());
      ClosePosition(position);
    } else if (!position.IsFullyOpened()) {
      ContinuePosition(position);
    } else {
      HoldPosition(position);
    }
  }
}

void PositionController::OnPostionsCloseRequest() {
  CloseAllPositions(CLOSE_REASON_REQUEST);
}

void PositionController::OnBrokerPositionUpdate(
    const boost::shared_ptr<PositionOperationContext> &operationContext,
    Security &security,
    bool isLong,
    const Qty &qty,
    const Volume &volume,
    bool isInitial) {
  if (!isInitial || qty == 0) {
    GetStrategy().GetLog().Debug(
        "Skipped broker position \"%5%\" %1% (volume %2$.8f) for \"%3%\" "
        "(%4%).",
        qty,                               // 1
        volume,                            // 2
        security,                          // 3
        isInitial ? "initial" : "online",  // 4
        isLong ? "long" : "short");        // 5
    return;
  }

  const Price price = volume / qty;
  GetStrategy().GetLog().Info(
      "Accepting broker position \"%5%\" %1% (volume %2$.8f, start price "
      "%3$.8f) for \"%4%\"...",
      qty,                         // 1
      volume,                      // 2
      price,                       // 3
      security,                    // 4
      isLong ? "long" : "short");  // 5
  RestorePosition(operationContext, security, isLong, qty, price, price,
                  nullptr, Milestones());
}

template <typename PositionType>
boost::shared_ptr<Position> PositionController::CreatePositionObject(
    const boost::shared_ptr<PositionOperationContext> &operationContext,
    Security &security,
    const Qty &qty,
    const Price &price,
    const TimeMeasurement::Milestones &delayMeasurement) {
  const auto &result = boost::make_shared<PositionType>(
      operationContext, GetStrategy(), GenerateNewOperationId(), 1,
      operationContext->GetTradingSystem(GetStrategy(), security), security,
      security.GetSymbol().GetCurrency(), qty, price, delayMeasurement);
  return result;
}

boost::shared_ptr<Position> PositionController::CreatePosition(
    const boost::shared_ptr<PositionOperationContext> &operationContext,
    bool isLong,
    Security &security,
    const Qty &qty,
    const Price &price,
    const Milestones &delayMeasurement) {
  const auto &result =
      isLong ? CreatePositionObject<LongPosition>(operationContext, security,
                                                  qty, price, delayMeasurement)
             : CreatePositionObject<ShortPosition>(
                   operationContext, security, qty, price, delayMeasurement);
  result->GetOperationContext().Setup(*result);
  return result;
}
