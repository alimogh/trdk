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
#include "TradingLib/Fwd.hpp"
#include "Fwd.hpp"

namespace trdk {
namespace Strategies {
namespace DocFeels {

class PositionController : public TradingLib::PositionController {
 public:
  typedef TradingLib::PositionController Base;

 public:
  explicit PositionController(Strategy &, const TradingLib::Trend &);
  virtual ~PositionController() override = default;

 public:
  virtual void OnPositionUpdate(trdk::Position &) override;

 public:
  using Base::OpenPosition;
  virtual trdk::Position &OpenPosition(
      trdk::Security &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;

 protected:
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy() const override;
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy() const override;
  virtual trdk::Qty GetNewPositionQty() const override;
  virtual bool IsPositionCorrect(const trdk::Position &) const override;
  virtual std::unique_ptr<TradingLib::PositionReport> OpenReport()
      const override;
  const PositionReport &GetReport() const;

 private:
  const boost::shared_ptr<const TradingLib::OrderPolicy> m_orderPolicy;
  const TradingLib::Trend &m_trend;
};
}
}
}