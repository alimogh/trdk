/*******************************************************************************
 *   Created: 2017/12/12 15:12:38
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

typedef std::string LivecoinProductId;

struct LivecoinProduct {
  LivecoinProductId id;
  std::string requestId;
  Volume minBtcVolume;
  uintmax_t pricePrecisionPower;
};

boost::unordered_map<std::string, LivecoinProduct> RequestLivecoinProductList(
    Poco::Net::HTTPClientSession &, const Context &, ModuleEventsLog &);
}
}
}