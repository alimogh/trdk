/**************************************************************************
 *   Created: 2012/07/22 23:40:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

namespace Trader { namespace Engine {

	//////////////////////////////////////////////////////////////////////////

	void Connect(
				Trader::TradeSystem &,
				const Trader::Lib::IniFile &,
				const std::string &section);

	void Connect(Trader::MarketDataSource &);

	//////////////////////////////////////////////////////////////////////////

	template<typename T>
	struct ModuleNamePolicy {
		//...//
	};
	
	template<>
	struct ModuleNamePolicy<Strategy> {
		static const char * GetName() {
			return "strategy";
		}
		static const char * GetNameFirstCapital() {
			return "Strategy";
		}
	};
	
	template<>
	struct ModuleNamePolicy<Observer> {
		static const char * GetName() {
			return "observer";
		}
		static const char * GetNameFirstCapital() {
			return "Observer";
		}
	};

	template<>
	struct ModuleNamePolicy<Service> {
		static const char * GetName() {
			return "service";
		}
		static const char * GetNameFirstCapital() {
			return "Service";
		}
	};

	//////////////////////////////////////////////////////////////////////////

} }
