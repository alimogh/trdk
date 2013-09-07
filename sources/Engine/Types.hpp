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

	typedef trdk::Lib::DllObjectPtr<Strategy> ModuleStrategy;
	typedef std::map<std::string /*tag*/, std::list<ModuleStrategy>> Strategies;

	typedef trdk::Lib::DllObjectPtr<Observer> ModuleObserver;
	typedef std::map<std::string /*tag*/, std::list<ModuleObserver>> Observers;

	typedef trdk::Lib::DllObjectPtr<Service> ModuleService;
	typedef std::map<std::string /*tag*/, std::list<ModuleService>> Services;

} }
