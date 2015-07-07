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

	struct TradeSystemHolder {
		std::string tag;
		// Deinitialization order is important!
		trdk::Lib::DllObjectPtr<trdk::TradeSystem> tradeSystem;
		std::string section;
		boost::shared_ptr<Terminal> terminal;
	};
	struct TradeSystemModesHolder {
		std::string tag;
		boost::array<TradeSystemHolder, numberOfTradingModes> holders;
	};
	typedef std::vector<TradeSystemModesHolder> TradeSystems;
	
	typedef std::vector<trdk::Lib::DllObjectPtr<MarketDataSource>>
		MarketDataSources;

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

	typedef trdk::Lib::DllObjectPtr<trdk::DropCopy> DropCopyModule;

} }
