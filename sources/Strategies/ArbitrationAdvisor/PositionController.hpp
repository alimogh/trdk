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

#include "Report.hpp"

namespace trdk {
namespace Strategies {
namespace ArbitrageAdvisor {

class PositionController : public TradingLib::PositionController {
 public:
  typedef TradingLib::PositionController Base;

 public:
  explicit PositionController(OperationReport &&);
  virtual ~PositionController() override = default;

 public:
  virtual void OnPositionUpdate(trdk::Position &) override;

  virtual bool ClosePosition(Position &, const CloseReason &) override;

 protected:
  virtual void HoldPosition(Position &) override;
  virtual void ClosePosition(Position &) override;

 private:
  OperationReport m_report;
};
}
}
}