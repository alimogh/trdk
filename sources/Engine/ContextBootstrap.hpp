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

	void BootstrapContext(
			const trdk::Lib::IniFile &,
			const Settings &,
			Context &,
			trdk::Lib::DllObjectPtr<TradeSystem> &tradeSystemRef,
			trdk::Lib::DllObjectPtr<MarketDataSource> &marketDataSourceRef);

	void BootstrapContextState(
				const trdk::Lib::IniFile &,
				Context &,
				SubscriptionsManager &subscriptionsManagerRef,
				Strategies &strategiesRef,
				Observers &observersRef,
				Services &servicesRef,
				ModuleList &moduleListRef);

} }
