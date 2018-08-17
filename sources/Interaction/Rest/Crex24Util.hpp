/*******************************************************************************
 *   Created: 2018/02/16 03:58:47
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

struct Crex24Product {
  std::string id;
  Price tickSize;
  Price minPrice;
  Volume minVolume;
};

boost::unordered_map<std::string, Crex24Product> RequestCrex24ProductList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &,
    const Context &,
    ModuleEventsLog &);

std::unique_ptr<Poco::Net::HTTPSClientSession> CreateCrex24Session(
    const Settings &, bool isTrading);

}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
