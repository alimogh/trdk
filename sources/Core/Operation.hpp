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
class TRDK_CORE_API Operation {
 public:
  explicit Operation(trdk::Strategy &, std::unique_ptr<trdk::PnlContainer> &&);
  Operation(Operation &&);
  virtual ~Operation();

 private:
  Operation(const Operation &);
  const Operation &operator=(const Operation &);

 public:
  const boost::uuids::uuid &GetId() const;
  const trdk::Strategy &GetStrategy() const;
  trdk::Strategy &GetStrategy();

 public:
  virtual trdk::TradingSystem &GetTradingSystem(trdk::Security &);
  virtual const trdk::TradingSystem &GetTradingSystem(
      const trdk::Security &) const;

  //! Order policy for position opening.
  virtual const trdk::TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const trdk::Position &) const;
  //! Order policy for position closing.
  virtual const trdk::TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const trdk::Position &) const;

  //! Setups new position.
  /** Place to attach stop-orders and so on.
    */
  virtual void Setup(trdk::Position &,
                     trdk::TradingLib::PositionController &) const;

  //! Next new position direction.
  virtual bool IsLong(const trdk::Security &) const;

  //! Next new position quantity.
  virtual trdk::Qty GetPlannedQty() const;

  //! Returns true if the opened position should be closed as soon as possible.
  virtual bool HasCloseSignal(const trdk::Position &) const;

  //! Will be called before each closing state changing.
  /** @return True, if reason can be changed, false otherwise.
    */
  virtual bool OnCloseReasonChange(trdk::Position &,
                                   const trdk::CloseReason &newReason);

  //! Returns parent operation or nullptr if doesn't have parent.
  virtual boost::shared_ptr<const trdk::Operation> GetParent() const;
  //! Returns parent operation or nullptr if doesn't have parent.
  virtual boost::shared_ptr<trdk::Operation> GetParent();

  //! Returns object for inverted position.
  /** @return Pointer to an operation for inverted position or empty pointer if
    *         the position can be inverted.
    */
  virtual boost::shared_ptr<trdk::Operation> StartInvertedPosition(
      const trdk::Position &);

  void UpdatePnl(const trdk::Security &, const trdk::Volume &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
