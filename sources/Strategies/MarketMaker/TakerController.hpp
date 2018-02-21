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
  typedef TradingLib::PositionController Base;

 public:
  virtual ~TakerController() override = default;

 protected:
  virtual void HoldPosition(trdk::Position &) override;
};

}  // namespace MarketMaker
}  // namespace Strategies
}  // namespace trdk