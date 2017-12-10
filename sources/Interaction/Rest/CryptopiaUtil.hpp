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
  Lib::Double feeRatio;
  std::pair<Qty, Qty> minMaxQty;
  std::pair<Qty, Qty> minMaxBaseQty;
  std::pair<Price, Price> minMaxPrice;
};

boost::unordered_map<std::string, CryptopiaProduct> RequestCryptopiaProductList(
    Poco::Net::HTTPClientSession &, Context &, ModuleEventsLog &);
}
}
}