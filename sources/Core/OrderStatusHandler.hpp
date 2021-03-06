/*******************************************************************************
 *   Created: 2018/01/09 16:10:58
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Fwd.hpp"

namespace trdk {
class OrderStatusHandler : private boost::noncopyable {
 public:
  virtual ~OrderStatusHandler() = default;

 public:
  //! Order sent to the trading system and received reception confirmation.
  //! The order is active.
  /** @sa ORDER_STATUS_OPENED
   */
  virtual void OnOpened() = 0;
  //! The order is fully filled and is not active anymore.
  /** @sa ORDER_STATUS_FILLED_FULLY
   */
  virtual void OnFilled(const trdk::Volume &comission) = 0;
  //! The order got a new part of the partial filling and still be active.
  /** @sa ORDER_STATUS_FILLED_PARTIALLY
   */
  virtual void OnTrade(const trdk::Trade &) = 0;
  //! The order is canceled by the owner with or without partial filling. The
  //! order is not active anymore.
  /** @sa ORDER_STATUS_CANCELED
   */
  virtual void OnCanceled(const trdk::Volume &comission) = 0;
  //! The order is rejected by the trading system and is not active.
  //! Remaining quantity is canceled.
  /** @sa ORDER_STATUS_REJECTED,
   */
  virtual void OnRejected(const trdk::Volume &comission) = 0;
  //! The unknown error has occurred. State of the order is unknown.
  /** @sa ORDER_STATUS_ERROR,
   */
  virtual void OnError(const trdk::Volume &comission) = 0;
};
}  // namespace trdk
