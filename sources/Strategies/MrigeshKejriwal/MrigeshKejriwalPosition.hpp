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

#include "Core/Position.hpp"

namespace trdk {
namespace Strategies {
namespace MrigeshKejriwal {

template <typename Base>
class StrategyPosition : public Base {
 public:
  explicit StrategyPosition(
      trdk::Strategy &strategy,
      const boost::uuids::uuid &operationId,
      int64_t subOperationId,
      TradingSystem &tradingSystem,
      Security &security,
      const Lib::Currency &currency,
      const Qty &qty,
      const Price &startPrice,
      const Lib::TimeMeasurement::Milestones &delayMeasurement)
      : Base(strategy,
             operationId,
             subOperationId,
             tradingSystem,
             security,
             currency,
             qty,
             startPrice,
             delayMeasurement),
        m_openSignalPrice(0),
        m_closeSignalPrice(0) {}
  virtual ~StrategyPosition() override = default;

 public:
  void SetOpenSignalPrice(const Price &price) {
    AssertEq(0, m_openSignalPrice);
    AssertLt(0, price);
    m_openSignalPrice = price;
  }
  void SetCloseSignalPrice(const Price &price) {
    AssertLt(0, price);
    if (m_closeSignalPrice != 0) {
      return;
    }
    m_closeSignalPrice = price;
  }

 public:
  const Price &GetOpenSignalPrice() const {
    AssertNe(0, m_openSignalPrice);
    return m_openSignalPrice;
  }
  const Price &GetCloseSignalPrice() const {
    AssertNe(0, m_closeSignalPrice);
    return m_closeSignalPrice;
  }

 private:
  Price m_openSignalPrice;
  Price m_closeSignalPrice;
};

typedef StrategyPosition<LongPosition> StrategyLongPosition;
typedef StrategyPosition<ShortPosition> StrategyShortPosition;
}
}
}
