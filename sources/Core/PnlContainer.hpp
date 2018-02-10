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
  virtual bool Update(const trdk::Security &,
                      const trdk::OrderSide &,
                      const trdk::Qty &,
                      const trdk::Price &,
                      const Volume &comission) = 0;
};
}  // namespace trdk
