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

namespace trdk { namespace Engine {

	void BootContext(
			const trdk::Lib::Ini &,
			Context &,
			TradeSystems &tradeSystemsRef,
			MarketDataSources &marketDataSourcesRef);

	void BootContextState(
			const trdk::Lib::Ini &,
			Context &,
			SubscriptionsManager &subscriptionsManagerRef,
			Strategies &strategiesRef,
			Observers &observersRef,
			Services &servicesRef,
			ModuleList &moduleListRef,
			DropCopyModule &dropCopyRef);

	void BootNewStrategiesForContextState(
			const trdk::Lib::Ini &newStrategiesConf,
			Context &,
			SubscriptionsManager &subscriptionsManagerRef,
			Strategies &strategiesRef,
			Services &servicesRef,
			ModuleList &moduleListRef);

} }
