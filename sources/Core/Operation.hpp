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
  explicit Operation(Strategy &, std::unique_ptr<PnlContainer> &&);
  Operation(Operation &&) noexcept;
  Operation(const Operation &) = delete;
  const Operation &operator=(Operation &&) = delete;
  const Operation &operator=(const Operation &) = delete;
  virtual ~Operation();

  const boost::uuids::uuid &GetId() const;
  const Strategy &GetStrategy() const;
  Strategy &GetStrategy();

  virtual TradingSystem &GetTradingSystem(Security &);
  virtual const TradingSystem &GetTradingSystem(const Security &) const;

  //! Order policy for position opening.
  virtual const TradingLib::OrderPolicy &GetOpenOrderPolicy(
      const Position &) const;
  //! Order policy for position closing.
  virtual const TradingLib::OrderPolicy &GetCloseOrderPolicy(
      const Position &) const;

  //! Setups new position.
  /** Place to attach stop-orders and so on.
   */
  virtual void Setup(Position &, TradingLib::PositionController &) const;

  //! Next new position direction.
  virtual bool IsLong(const Security &) const;

  //! Next new position quantity.
  virtual Qty GetPlannedQty(const Security &) const;

  //! Returns true if the opened position should be closed as soon as possible.
  virtual bool HasCloseSignal(const Position &) const;

  //! Will be called before each closing state changing.
  /** @return True, if reason can be changed, false otherwise.
   */
  virtual bool OnCloseReasonChange(Position &, const CloseReason &newReason);

  //! Returns parent operation or nullptr if doesn't have parent.
  virtual boost::shared_ptr<const Operation> GetParent() const;
  //! Returns parent operation or nullptr if doesn't have parent.
  virtual boost::shared_ptr<Operation> GetParent();

  //! Returns object for inverted position.
  /** @return Pointer to an operation for inverted position or empty pointer if
   *          the position can be inverted.
   */
  virtual boost::shared_ptr<trdk::Operation> StartInvertedPosition(
      const trdk::Position &);

  const trdk::Pnl &GetPnl() const;

  void UpdatePnl(const Security &,
                 const OrderSide &,
                 const Qty &,
                 const Price &);
  void UpdatePnl(const Security &,
                 const OrderSide &,
                 const Qty &,
                 const Price &,
                 const Volume &commission);
  void AddCommission(const Security &, const Volume &commission);

  void OnNewPositionStart(Position &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}  // namespace trdk
