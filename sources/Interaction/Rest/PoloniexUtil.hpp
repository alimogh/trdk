/*******************************************************************************
 *   Created: 2017/11/30 22:57:19
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

struct PoloniexProduct {
  std::string id;
};

boost::unordered_map<std::string, PoloniexProduct> RequestPoloniexProductList(
    Poco::Net::HTTPClientSession &, Context &, ModuleEventsLog &);
}
}
}
