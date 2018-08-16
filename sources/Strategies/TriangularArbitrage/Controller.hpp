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

  ~Controller() override = default;

  void OnUpdate(Position &) override;
  using Base::Close;

 protected:
  void Hold(Position &) override;
  void Close(Position &) override;
};

}  // namespace TriangularArbitrage
}  // namespace Strategies
}  // namespace trdk
