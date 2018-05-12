/*******************************************************************************
 *   Created: 2018/01/09 16:12:16
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {

//! Trade.
class Trade {
 public:
  //! Trade price.
  /** If set as "zero" - will be calculated automatically by trading system.
   */
  Price price;
  //! Trade quantity.
  /** If set as "zero" - will be calculated automatically by trading system.
   */
  Qty qty;
  //! Trade ID. Optional.
  std::string id;
};
}  // namespace trdk
