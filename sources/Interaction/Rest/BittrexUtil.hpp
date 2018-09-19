/*******************************************************************************
 *   Created: 2017/11/19 18:24:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#pragma once

#include "Settings.hpp"

namespace trdk {
namespace Interaction {
namespace Rest {

struct BittrexProduct {
  std::string id;
  Qty minQty;
};

boost::unordered_map<std::string, BittrexProduct> RequestBittrexProductList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &,
    const Context &,
    ModuleEventsLog &);

struct BittrexSettings : Settings {
  typedef Settings Base;

  std::string apiKey;
  std::string apiSecret;

  explicit BittrexSettings(const boost::property_tree::ptree &,
                           ModuleEventsLog &);
};

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk