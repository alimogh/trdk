/*******************************************************************************
 *   Created: 2018/02/21 15:47:51
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
namespace MarketMaker {

class TakerController : public TradingLib::PositionController {
 public:
  typedef PositionController Base;

  ~TakerController() override = default;

  void OnPositionUpdate(Position &) override;

 protected:
  void HoldPosition(Position &) override;
};

}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk