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
  std::pair<Qty, Qty> minMaxVolume;
  std::pair<Price, Price> minMaxPrice;

  Price NormalizePrice(const trdk::Price &price,
                       const trdk::Security &security) const {
    return !isReversed ? price : TradingLib::ReversePrice(price, security);
  }
  Qty NormalizeQty(const trdk::Price &price,
                   const trdk::Qty &qty,
                   const trdk::Security &security) const {
    return !isReversed ? qty : TradingLib::ReverseQty(price, qty, security);
  }
};

boost::unordered_map<std::string, CryptopiaProduct> RequestCryptopiaProductList(
    Poco::Net::HTTPClientSession &, Context &, ModuleEventsLog &);
}
}
}