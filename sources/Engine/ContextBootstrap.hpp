/**************************************************************************
 *   Created: 2013/05/20 01:22:48
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Fwd.hpp"

namespace trdk {
namespace Engine {

std::pair<std::vector<Lib::DllObjectPtr<TradingSystem>>,
          std::vector<Lib::DllObjectPtr<MarketDataSource>>>
LoadSources(trdk::Context &);

std::vector<Lib::DllObjectPtr<Strategy>> LoadStrategies(
    const boost::property_tree::ptree &,
    trdk::Context &,
    SubscriptionsManager &);

std::vector<Lib::DllObjectPtr<Strategy>> LoadStrategies(trdk::Context &,
                                                        SubscriptionsManager &);

}  // namespace Engine
}  // namespace trdk
