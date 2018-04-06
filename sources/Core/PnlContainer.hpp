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

class PnlContainer : public Pnl {
 public:
  ~PnlContainer() override = default;

  virtual void UpdateFinancialResult(const Security &,
                                     const OrderSide &,
                                     const Qty &,
                                     const Price &) = 0;
  virtual void UpdateFinancialResult(const Security &,
                                     const OrderSide &,
                                     const Qty &,
                                     const Price &,
                                     const Volume &commission) = 0;
  virtual void AddCommission(const Security &, const Volume &) = 0;
};

}  // namespace trdk
