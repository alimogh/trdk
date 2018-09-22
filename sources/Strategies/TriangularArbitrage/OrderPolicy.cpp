/*******************************************************************************
 *   Created: 2018/03/10 03:53:32
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "OrderPolicy.hpp"

using namespace trdk;
using namespace Strategies;
using namespace TriangularArbitrage;

Price TriangularArbitrage::CorrectMarketPriceToOrderPrice(
    const Price &marketPrice, const bool isBuy) {
  return marketPrice * (isBuy ? 1.03 : 0.97);
}

Price OrderPolicy::GetOpenOrderPrice(Position &position) const {
  return CorrectMarketPriceToOrderPrice(Base::GetOpenOrderPrice(position),
                                        position.IsLong());
}

Price OrderPolicy::GetCloseOrderPrice(Position &position) const {
  return CorrectMarketPriceToOrderPrice(Base::GetCloseOrderPrice(position),
                                        !position.IsLong());
}
