/**************************************************************************
 *   Created: 2013/01/26 11:56:08
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 **************************************************************************/

#pragma once

namespace trdk {
	
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
			return this->get_pointer() == rhs.get_pointer();
		}
	};

	typedef trdk::ModuleReferenceWrapper<const trdk::Strategy>
		ConstStrategyRefWrapper;
	typedef trdk::ModuleReferenceWrapper<trdk::Strategy>
		StrategyRefWrapper;
		
	typedef trdk::ModuleReferenceWrapper<const trdk::Service>
		ConstServiceRefWrapper;
	typedef trdk::ModuleReferenceWrapper<trdk::Service>
		ServiceRefWrapper;
		
	typedef trdk::ModuleReferenceWrapper<const trdk::Observer>
		ConstObserverRefWrapper;
	typedef trdk::ModuleReferenceWrapper<trdk::Observer>
		ObserverRefWrapper;

	template<typename Module>
	inline trdk::ModuleReferenceWrapper<const Module> ConstModuleRef(
				const Module &module) {
		static_assert(
			boost::is_base_of<trdk::Module, Module>::value,
			"Template parameter must be derived from trdk::Module.");
		return trdk::ModuleReferenceWrapper<const Module>(module);
	}

	template<typename Module>
	inline trdk::ModuleReferenceWrapper<Module> ModuleRef(
				Module &module) {
		static_assert(
			boost::is_base_of<trdk::Module, Module>::value,
			"Template parameter must be derived from trdk::Module.");
		return trdk::ModuleReferenceWrapper<Module>(module);
	}

}
