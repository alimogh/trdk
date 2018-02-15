/*******************************************************************************
 *   Created: 2017/12/17 23:06:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Util.hpp"

using namespace trdk;
using namespace trdk::Lib;

Price TradingLib::ReversePrice(const Price &price, const Security &security) {
  if (!price) {
    return 0;
  }
  return RoundByPrecisionPower(1 / price, security.GetPricePrecisionPower());
}

Qty TradingLib::ReverseQty(const Price &price,
                           const Qty &qty,
                           const Security &security) {
  return RoundByPrecisionPower(price * qty, security.GetPricePrecisionPower());
}
