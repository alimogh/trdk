/*******************************************************************************
 *   Created: 2018/01/23 11:04:09
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Pnl.hpp"

namespace trdk {

class PnlContainer : public trdk::Pnl {
 public:
  virtual ~PnlContainer() = default;

 public:
  virtual void UpdateFinancialResult(const trdk::Security &,
                                     const trdk::OrderSide &,
                                     const trdk::Qty &,
                                     const trdk::Price &) = 0;
  virtual void UpdateFinancialResult(const trdk::Security &,
                                     const trdk::OrderSide &,
                                     const trdk::Qty &,
                                     const trdk::Price &,
                                     const trdk::Volume &commission) = 0;
  virtual void AddCommission(const trdk::Security &, const trdk::Volume &) = 0;
};

}  // namespace trdk
