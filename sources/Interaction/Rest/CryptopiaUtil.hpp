/*******************************************************************************
 *   Created: 2017/12/07 15:16:01
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

namespace trdk {
namespace Interaction {
namespace Rest {

typedef int CryptopiaProductId;

struct CryptopiaProduct {
  CryptopiaProductId id;
  bool isReversed;
  Lib::Double feeRatio;
  std::pair<Qty, Qty> minMaxQty;
  std::pair<Volume, Volume> minMaxVolume;
  std::pair<Price, Price> minMaxPrice;

  Price NormalizePrice(const Price &price,
                       const trdk::Security &security) const {
    return !isReversed ? price : TradingLib::ReversePrice(price, security);
  }
  Qty NormalizeQty(const Price &price,
                   const Qty &qty,
                   const trdk::Security &security) const {
    return !isReversed ? qty : TradingLib::ReverseQty(price, qty, security);
  }
  OrderSide NormalizeSide(const OrderSide &side) const {
    return !isReversed
               ? side
               : side == ORDER_SIDE_BUY ? ORDER_SIDE_SELL : ORDER_SIDE_BUY;
  }
};

boost::unordered_map<std::string, CryptopiaProduct> RequestCryptopiaProductList(
    Poco::Net::HTTPClientSession &, Context &, ModuleEventsLog &);
}
}
}