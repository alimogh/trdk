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

 public:
  explicit PositionController(Strategy &);
  virtual ~PositionController() override;

 public:
  using Base::ClosePosition;

 protected:
  virtual void HoldPosition(Position &) override;
  virtual void ClosePosition(trdk::Position &) override;

 private:
  Position &GetOppositePosition(Position &);

 private:
  std::unique_ptr<Report> m_report;
};
}
}
}