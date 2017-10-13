/*******************************************************************************
 *   Created: 2017/10/12 15:10:34
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "TradingLib/Fwd.hpp"
#include "Api.h"

namespace trdk {

//! Describes one or more operations with position.
class TRDK_CORE_API PositionOperationContext : private boost::noncopyable {
 public:
  virtual ~PositionOperationContext() = default;

 public:
  //! Order policy for position opening.
  virtual const trdk::TradingLib::OrderPolicy &GetOpenOrderPolicy() const = 0;
  //! Order policy for position closing.
  virtual const trdk::TradingLib::OrderPolicy &GetCloseOrderPolicy() const = 0;
  //! Setups new position.
  /** Place to attach stop-orders and so on.
    */
  virtual void Setup(trdk::Position &) const = 0;
  //! Next new position direction.
  virtual bool IsLong() const = 0;
  //! Next new position quantity.
  virtual trdk::Qty GetPlannedQty() const = 0;
  //! Returns true if the opened position should be closed as soon as possible.
  virtual bool HasCloseSignal(const trdk::Position &) const = 0;
  //! Is the next position should be inverted to opposite side.
  virtual bool IsInvertible(const trdk::Position &) const = 0;
  //! Will be called before each closing state changing.
  virtual void OnCloseReasonChange(const trdk::CloseReason &prevReason,
                                   const trdk::CloseReason &newReason);
};
}
