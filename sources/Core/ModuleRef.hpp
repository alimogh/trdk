/**************************************************************************
 *   Created: 2013/01/26 11:56:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 **************************************************************************/

#pragma once

#include "Fwd.hpp"

namespace Trader {
	
	template<typename Module>
	class ModuleReferenceWrapper : public boost::reference_wrapper<Module> {
	public:
		typedef boost::reference_wrapper<Module> Base;
	public:
		explicit ModuleReferenceWrapper(Module &moduleRef)
				: Base(moduleRef) {
			//...//
		}
	public:
		bool operator ==(const ModuleReferenceWrapper &rhs) const {
			return get_pointer() == rhs.get_pointer();
		}
	};

	typedef Trader::ModuleReferenceWrapper<const Trader::Strategy>
		ConstStrategyRefWrapper;
	typedef Trader::ModuleReferenceWrapper<Trader::Strategy>
		StrategyRefWrapper;
		
	typedef Trader::ModuleReferenceWrapper<const Trader::Service>
		ConstServiceRefWrapper;
	typedef Trader::ModuleReferenceWrapper<Trader::Service>
		ServiceRefWrapper;
		
	typedef Trader::ModuleReferenceWrapper<const Trader::Observer>
		ConstObserverRefWrapper;
	typedef Trader::ModuleReferenceWrapper<Trader::Observer>
		ObserverRefWrapper;

	template<typename Module>
	inline Trader::ModuleReferenceWrapper<const Module> ConstModuleRef(
				const Module &module) {
		static_assert(
			boost::is_base_of<Trader::Module, Module>::value,
			"Template parameter must be derived from Trader::Module.");
		return Trader::ModuleReferenceWrapper<const Module>(module);
	}

	template<typename Module>
	inline Trader::ModuleReferenceWrapper<Module> ModuleRef(
				Module &module) {
		static_assert(
			boost::is_base_of<Trader::Module, Module>::value,
			"Template parameter must be derived from Trader::Module.");
		return Trader::ModuleReferenceWrapper<Module>(module);
	}

}
