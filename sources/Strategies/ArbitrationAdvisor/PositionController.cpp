/*******************************************************************************
 *   Created: 2017/11/07 00:01:40
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "PositionController.hpp"
#include "Operation.hpp"
#include "Strategy.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Strategies::ArbitrageAdvisor;

namespace aa = trdk::Strategies::ArbitrageAdvisor;

aa::PositionController::PositionController(Strategy &strategy)
    : Base(strategy),
      m_report(boost::make_unique<OperationReport>(GetStrategy())) {}

void aa::PositionController::OnPositionUpdate(Position &position) {
  if (position.IsCompleted()) {
    auto &reportData =
        position.GetTypedOperation<aa::Operation>().GetReportData();
    if (reportData.Add(position)) {
      m_report->Append(reportData);
    }
    return;
  }

  Base::OnPositionUpdate(position);
}

void aa::PositionController::HoldPosition(Position &position) {
  Assert(position.IsFullyOpened());
  Assert(!position.IsCompleted());
  Assert(!position.HasActiveOrders());

  Position *const oppositePosition = FindOppositePosition(position);
  if (!oppositePosition) {
    ClosePosition(position, CLOSE_REASON_SYSTEM_ERROR);
    return;
  }
  Assert(!position.IsCompleted());

  if (!oppositePosition->IsFullyOpened()) {
    // Waiting until another leg will be completed.
    return;
  }

  // Operation is completed.
  position.MarkAsCompleted();
  oppositePosition->MarkAsCompleted();
}

bool aa::PositionController::ClosePosition(Position &position,
                                           const CloseReason &reason) {
  boost::shared_future<void> oppositePositionClosingFuture;
  std::unique_ptr<const Strategy::PositionListTransaction> listTransaction;
  {
    auto *const oppositePosition = FindOppositePosition(position);
    if (oppositePosition &&
        oppositePosition->GetCloseReason() == CLOSE_REASON_NONE) {
      listTransaction =
          position.GetStrategy().StartThreadPositionsTransaction();
      oppositePositionClosingFuture =
          boost::async([this, &oppositePosition, &reason] {
            try {
              Base::ClosePosition(*oppositePosition, reason);
            } catch (const Interactor::CommunicationError &ex) {
              throw boost::enable_current_exception(ex);
            } catch (const Exception &ex) {
              throw boost::enable_current_exception(ex);
            } catch (const std::exception &ex) {
              throw boost::enable_current_exception(ex);
            }
          });
    }
  }
  const auto &result = Base::ClosePosition(position, reason);

  if (oppositePositionClosingFuture.valid()) {
    oppositePositionClosingFuture.get();
  }
  return result;
}

namespace {
class BestSecurityChecker : private boost::noncopyable {
 public:
  explicit BestSecurityChecker(Position &position)
      : m_position(position), m_bestSecurity(nullptr) {}

  virtual ~BestSecurityChecker() {
    if (!m_bestSecurity) {
      return;
    }
    try {
      m_position.ReplaceTradingSystem(
          *m_bestSecurity,
          m_position.GetOperation()->GetTradingSystem(m_position.GetStrategy(),
                                                      *m_bestSecurity));
    } catch (...) {
      AssertFailNoException();
      terminate();
    }
  }

 public:
  void Check(Security &checkSecurity) {
    AssertNe(m_bestSecurity, &checkSecurity);
    AssertEq(GetPosition().GetSecurity().GetSymbol().GetSymbol(),
             checkSecurity.GetSymbol().GetSymbol());
    if (!CheckGeneral(checkSecurity) || !CheckExchange(checkSecurity) ||
        !(!m_bestSecurity || CheckPrice(*m_bestSecurity, checkSecurity))) {
      return;
    }
    m_bestSecurity = &checkSecurity;
  }

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const = 0;
  virtual Qty GetQty(const Security &) const = 0;
  virtual Price GetPrice(const Security &) const = 0;
  virtual const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const = 0;
  virtual Double GetRequiredBalance() const = 0;
  virtual OrderSide GetSide() const = 0;

  const Position &GetPosition() const { return m_position; }

 private:
  bool CheckGeneral(const Security &checkSecurity) const {
    return checkSecurity.IsOnline() &&
           GetQty(checkSecurity) >= m_position.GetActiveQty();
  }

  bool CheckExchange(const Security &checkSecurity) const {
    const auto &tradingSystem = m_position.GetStrategy().GetTradingSystem(
        checkSecurity.GetSource().GetIndex());
    if (tradingSystem.GetBalances().FindAvailableToTrade(
            GetBalanceSymbol(checkSecurity)) < GetRequiredBalance()) {
      return false;
    }
    if (tradingSystem.CheckOrder(checkSecurity, m_position.GetCurrency(),
                                 m_position.GetActiveQty(),
                                 GetPrice(checkSecurity), GetSide())) {
      return false;
    }
    return true;
  }

 private:
  Position &m_position;
  Security *m_bestSecurity;
};

class BestSecurityCheckerForLongPosition : public BestSecurityChecker {
 public:
  explicit BestSecurityCheckerForLongPosition(Position &position)
      : BestSecurityChecker(position) {
    Assert(position.IsLong());
  }
  virtual ~BestSecurityCheckerForLongPosition() override = default;

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const override {
    return bestSecurity.GetBidPrice() < GetPrice(checkSecurity);
  }
  virtual Qty GetQty(const Security &checkSecurity) const override {
    return checkSecurity.GetBidQty();
  }
  virtual Price GetPrice(const Security &checkSecurity) const {
    return checkSecurity.GetBidPrice();
  }
  virtual const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const {
    return checkSecurity.GetSymbol().GetBaseSymbol();
  }
  virtual Double GetRequiredBalance() const {
    return GetPosition().GetActiveQty();
  }
  virtual OrderSide GetSide() const { return ORDER_SIDE_SELL; }
};

class BestSecurityCheckerForShortPosition : public BestSecurityChecker {
 public:
  explicit BestSecurityCheckerForShortPosition(Position &position)
      : BestSecurityChecker(position) {
    Assert(!position.IsLong());
  }
  virtual ~BestSecurityCheckerForShortPosition() override = default;

 protected:
  virtual bool CheckPrice(const Security &bestSecurity,
                          const Security &checkSecurity) const override {
    return bestSecurity.GetAskPrice() > GetPrice(checkSecurity);
  }
  virtual Qty GetQty(const Security &checkSecurity) const override {
    return checkSecurity.GetAskQty();
  }
  virtual Price GetPrice(const Security &checkSecurity) const {
    return checkSecurity.GetAskPrice();
  }
  virtual const std::string &GetBalanceSymbol(
      const Security &checkSecurity) const {
    return checkSecurity.GetSymbol().GetQuoteSymbol();
  }
  virtual Double GetRequiredBalance() const {
    return GetPosition().GetActiveVolume();
  }
  virtual OrderSide GetSide() const { return ORDER_SIDE_BUY; }
};

std::unique_ptr<BestSecurityChecker> CreateBestSecurityChecker(
    Position &positon) {
  if (positon.IsLong()) {
    return boost::make_unique<BestSecurityCheckerForLongPosition>(positon);
  } else {
    return boost::make_unique<BestSecurityCheckerForShortPosition>(positon);
  }
}
}

void aa::PositionController::ClosePosition(Position &position) {
  {
    const auto &checker = CreateBestSecurityChecker(position);
    boost::polymorphic_cast<aa::Strategy *>(&position.GetStrategy())
        ->ForEachSecurity(
            position.GetSecurity().GetSymbol(),
            [&checker](Security &security) { checker->Check(security); });
  }
  Base::ClosePosition(position);
}

Position *aa::PositionController::FindOppositePosition(
    const Position &position) {
  Position *result = nullptr;
  for (auto &oppositePosition : GetStrategy().GetPositions()) {
    if (oppositePosition.GetOperation() == position.GetOperation() &&
        &oppositePosition != &position) {
      Assert(!result);
      result = &oppositePosition;
#ifndef BOOST_ENABLE_ASSERT_HANDLER
      break;
#endif
    }
  }
  return result;
}
