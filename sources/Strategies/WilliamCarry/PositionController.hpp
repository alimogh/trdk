/*******************************************************************************
 *   Created: 2017/11/09 12:20:05
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
namespace WilliamCarry {

class PositionController : public TradingLib::PositionController {
 public:
  typedef TradingLib::PositionController Base;

 public:
  explicit PositionController(trdk::Strategy &);
  virtual ~PositionController() override = default;

 public:
  virtual void OnPositionUpdate(Position &) override;
  virtual void OnPostionsCloseRequest() override;

 protected:
  virtual void HoldPosition(trdk::Position &) override;
};
}
}
}