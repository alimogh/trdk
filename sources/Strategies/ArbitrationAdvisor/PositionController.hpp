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

class PositionController : public TradingLib::PositionController {
 public:
  typedef TradingLib::PositionController Base;

  ~PositionController() override = default;

  void OnUpdate(trdk::Position &) override;
  using Base::Close;

 protected:
  void Hold(Position &) override;
  void Close(Position &) override;
};
}  // namespace ArbitrageAdvisor
}  // namespace Strategies
}  // namespace trdk