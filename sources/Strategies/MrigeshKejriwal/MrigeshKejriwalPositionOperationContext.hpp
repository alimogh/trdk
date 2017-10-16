/*******************************************************************************
 *   Created: 2017/10/08 10:33:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Core/PositionOperationContext.hpp"

namespace trdk {
namespace Strategies {
namespace MrigeshKejriwal {

class Trend;
struct Settings;

class PositionOperationContext : public trdk::PositionOperationContext {
 public:
  explicit PositionOperationContext(const MrigeshKejriwal::Settings &,
                                    const Trend &,
                                    const Price &openSignalPrice);
  virtual ~PositionOperationContext() override = default;

 public:
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy() const override;
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy() const override;

  virtual void Setup(Position &position) const override;

  virtual bool IsLong() const override;

  virtual Qty GetPlannedQty() const override;

  virtual bool HasCloseSignal(const Position &) const;

  virtual void OnCloseReasonChange(const CloseReason &,
                                   const CloseReason &) override;

  virtual boost::shared_ptr<trdk::PositionOperationContext>
  StartInvertedPosition(const trdk::Position &) override;

 public:
  void SetCloseSignalPrice(const Price &);

  const Price &GetOpenSignalPrice() const;
  const Price &GetCloseSignalPrice() const;

 private:
  const MrigeshKejriwal::Settings &m_settings;
  const Trend &m_trend;

  Price m_openSignalPrice;
  Price m_closeSignalPrice;
};
}
}
}
