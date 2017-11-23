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
class TRDK_CORE_API PositionOperationContext
    : public boost::enable_shared_from_this<trdk::PositionOperationContext> {
 public:
  PositionOperationContext() = default;
  PositionOperationContext(PositionOperationContext &&) = default;
  virtual ~PositionOperationContext() = default;

 private:
  PositionOperationContext(const PositionOperationContext &);
  const PositionOperationContext &operator=(const PositionOperationContext &);

 public:
  virtual trdk::TradingSystem &GetTradingSystem(trdk::Strategy &,
                                                trdk::Security &);
  virtual const trdk::TradingSystem &GetTradingSystem(
      const trdk::Strategy &, const trdk::Security &) const;

  //! Order policy for position opening.
  virtual const trdk::TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const trdk::Position &) const = 0;
  //! Order policy for position closing.
  virtual const trdk::TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const trdk::Position &) const = 0;

  //! Setups new position.
  /** Place to attach stop-orders and so on.
    */
  virtual void Setup(trdk::Position &) const = 0;

  //! Next new position direction.
  virtual bool IsLong(const trdk::Security &) const = 0;

  //! Next new position quantity.
  virtual trdk::Qty GetPlannedQty() const = 0;

  //! Returns true if the opened position should be closed as soon as possible.
  virtual bool HasCloseSignal(const trdk::Position &) const = 0;

  //! Will be called before each closing state changing.
  /** @return True, if reason can be changed, false otherwise.
    */
  virtual bool OnCloseReasonChange(trdk::Position &,
                                   const trdk::CloseReason &newReason);

  //! Returns parent operation or nullptr if doesn't have parent.
  virtual const PositionOperationContext *GetParent() const;
  //! Returns parent operation or nullptr if doesn't have parent.
  virtual PositionOperationContext *GetParent();

  //! Returns object for inverted position.
  /** @return Pointer to an operation for inverted position or empty pointer if
    *         the position can be inverted.
    */
  virtual boost::shared_ptr<PositionOperationContext> StartInvertedPosition(
      const trdk::Position &) = 0;
};
}
