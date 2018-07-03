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
  std::string symbol;
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
               : side == +OrderSide::Buy ? OrderSide::Sell : OrderSide::Buy;
  }
};

struct BySymbol {};
struct ById {};

typedef boost::multi_index_container<
    CryptopiaProduct,
    boost::multi_index::indexed_by<
        boost::multi_index::hashed_unique<
            boost::multi_index::tag<BySymbol>,
            boost::multi_index::member<CryptopiaProduct,
                                       std::string,
                                       &CryptopiaProduct::symbol>>,
        boost::multi_index::hashed_unique<
            boost::multi_index::tag<ById>,
            boost::multi_index::member<CryptopiaProduct,
                                       CryptopiaProductId,
                                       &CryptopiaProduct::id>>>>
    CryptopiaProductList;

inline size_t hash_value(const CryptopiaProductList::iterator &iterator) {
  return boost::hash_value(iterator->id);
}

CryptopiaProductList RequestCryptopiaProductList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &,
    const Context &,
    ModuleEventsLog &);
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk