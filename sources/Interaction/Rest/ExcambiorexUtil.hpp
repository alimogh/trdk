/*******************************************************************************
 *   Created: 2018/02/11 18:10:35
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

typedef std::string ExcambiorexProductId;

struct ExcambiorexProduct {
  ExcambiorexProductId id;
  std::string symbol;
};

std::pair<boost::unordered_map<std::string, ExcambiorexProduct>,
          boost::unordered_map<std::string, std::string>>
RequestExcambiorexProductAndCurrencyList(
    std::unique_ptr<Poco::Net::HTTPSClientSession> &,
    const Context &,
    ModuleEventsLog &);

std::unique_ptr<Poco::Net::HTTPSClientSession> CreateExcambiorexSession(
    const Settings &, bool isTrading);
}  // namespace Rest
}  // namespace Interaction
}  // namespace trdk
