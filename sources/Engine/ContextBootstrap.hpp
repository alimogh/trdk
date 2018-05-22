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

#include "Context.hpp"
#include "SubscriptionsManager.hpp"
#include "Types.hpp"

namespace trdk {
namespace Engine {

void BootContext(const Lib::Ini &,
                 Context &,
                 TradingSystems &tradingSystemsRef,
                 MarketDataSources &marketDataSourcesRef);

void BootContextState(const Lib::Ini &,
                      Context &,
                      SubscriptionsManager &subscriptionsManagerRef,
                      Strategies &strategiesRef,
                      ModuleList &moduleListRef);

void BootNewStrategiesForContextState(
    const Lib::Ini &newStrategiesConf,
    Context &,
    SubscriptionsManager &subscriptionsManagerRef,
    Strategies &strategiesRef,
    ModuleList &moduleListRef);

}  // namespace Engine
}  // namespace trdk
