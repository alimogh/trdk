/*******************************************************************************
 *   Created: 2017/11/06 00:08:39
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

////////////////////////////////////////////////////////////////////////////////

inline bool IsBusinessPosition(const Position &position) {
  if (position.GetSubOperationId() <= 2) {
    return true;
  }
  // In the current implementation "not business position" will be closed
  // immediately after creation.
  AssertNe(CLOSE_REASON_NONE, position.GetCloseReason());
  return false;
}

////////////////////////////////////////////////////////////////////////////////

class PositionController : public TradingLib::PositionController {
 public:
  typedef TradingLib::PositionController Base;

 public:
  explicit PositionController(Strategy &);
  virtual ~PositionController() override = default;

 public:
  virtual void OnPositionUpdate(trdk::Position &) override;
  using Base::ClosePosition;

 protected:
  virtual void HoldPosition(Position &) override;
  virtual void ClosePosition(trdk::Position &) override;

  Position *FindOppositePosition(const Position &);
  virtual void CompleteBusinessOperation(Position &lastLeg,
                                         Position *legBeforeLast);

 private:
  std::unique_ptr<BusinessOperationReport> m_report;
};

////////////////////////////////////////////////////////////////////////////////

class PositionAndBalanceController : public PositionController {
 public:
  typedef PositionController Base;

 public:
  explicit PositionAndBalanceController(Strategy &);
  virtual ~PositionAndBalanceController() override = default;

 public:
  virtual void OnPositionUpdate(trdk::Position &) override;

 protected:
  virtual std::unique_ptr<TradingLib::PositionReport> OpenReport()
      const override;
  virtual void CompleteBusinessOperation(Position &lastLeg,
                                         Position *legBeforeLast) override;

 private:
  void RestoreBalance(Position &position, Position *oppositePosition);
};

////////////////////////////////////////////////////////////////////////////////
}
}
}