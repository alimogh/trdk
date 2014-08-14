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

namespace trdk { namespace Engine {

	typedef std::vector<trdk::Lib::DllObjectPtr<MarketDataSource>>
		MarketDataSources;

	typedef std::map<
			std::string /*tag*/,
			std::vector<boost::shared_ptr<Strategy>>>
		Strategies;

	typedef std::map<
			std::string /*tag*/,
			std::vector<boost::shared_ptr<Observer>>>
		Observers;

	typedef std::map<
			std::string /*tag*/,
			std::vector<boost::shared_ptr<Service>>>
		Services;

	typedef std::set<boost::shared_ptr<trdk::Lib::Dll>> ModuleList;

} }
