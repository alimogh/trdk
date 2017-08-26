/*******************************************************************************
 *   Created: 2017/08/26 22:35:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once
#include "TradingLib/PositionController.hpp"
#include "Fwd.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class PositionController : public TradingLib::PositionController {
 public:
  typedef TradingLib::PositionController Base;

 public:
  explicit PositionController(Strategy &, const Trend &);
  virtual ~PositionController() override = default;

 public:
  virtual trdk::Position &OpenPosition(
      trdk::Security &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;
  virtual trdk::Position &OpenPosition(
      trdk::Security &,
      bool isLong,
      const trdk::Lib::TimeMeasurement::Milestones &) override;
  virtual void ContinuePosition(trdk::Position &) override;
  virtual void ClosePosition(trdk::Position &,
                             const trdk::CloseReason &) override;

 protected:
  virtual bool IsPositionCorrect(const trdk::Position &) const override;

 private:
  const Trend &m_trend;
};
}
}
}