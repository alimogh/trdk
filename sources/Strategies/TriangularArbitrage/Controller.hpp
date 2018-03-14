/*******************************************************************************
 *   Created: 2018/03/06 11:00:47
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
namespace TriangularArbitrage {

class Controller : public TradingLib::PositionController {
 public:
  typedef PositionController Base;

 public:
  virtual ~Controller() override = default;

 public:
  virtual void OnPositionUpdate(trdk::Position &) override;
  using Base::ClosePosition;

 protected:
  virtual void HoldPosition(trdk::Position &) override;
  virtual void ClosePosition(Position &) override;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk
