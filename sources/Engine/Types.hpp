/**************************************************************************
 *   Created: 2013/05/20 01:41:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/DropCopy.hpp"

namespace trdk { namespace Engine {

	struct TradingSystemHolder {
		std::string tag;
		// Deinitialization order is important!
		trdk::Lib::DllObjectPtr<trdk::TradingSystem> tradingSystem;
		std::string section;
		boost::shared_ptr<Terminal> terminal;
	};
	struct TradingSystemModesHolder {
		std::string tag;
		static_assert(numberOfTradingModes == 3, "List changed.");
		boost::array<TradingSystemHolder, 2> holders;
	};
	typedef std::vector<TradingSystemModesHolder> TradingSystems;
	
	struct MarketDataSourceHolder {
		const std::string section;
		trdk::Lib::DllObjectPtr<MarketDataSource> marketDataSource;
	};
	typedef std::vector<MarketDataSourceHolder> MarketDataSources;

	template<typename Module>
	struct ModuleHolder {
		std::string section;
		boost::shared_ptr<Module> module;
	};

	typedef std::map<
			std::string /*tag*/,
			std::vector<ModuleHolder<Strategy>>>
		Strategies;

	typedef std::map<
			std::string /*tag*/,
			std::vector<ModuleHolder<Observer>>>
		Observers;

	typedef std::map<
			std::string /*tag*/,
			std::vector<ModuleHolder<Service>>>
		Services;

	typedef std::set<boost::shared_ptr<trdk::Lib::Dll>> ModuleList;

} }
