/**************************************************************************
 *   Created: 2013/05/20 01:49:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "ContextBootstrap.hpp"
#include "Ini.hpp"
#include "Core/MarketDataSource.hpp"
#include "Core/TradeSystem.hpp"
#include "Core/Strategy.hpp"
#include "Core/Observer.hpp"
#include "Core/Service.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;
namespace mi = boost::multi_index;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;

////////////////////////////////////////////////////////////////////////////////

class ContextBootstraper : private boost::noncopyable {

public:

	explicit ContextBootstraper(
				const IniFile &conf,
				const Settings &settings,
				Engine::Context &context,
				DllObjectPtr<TradeSystem> &tradeSystemRef,
				DllObjectPtr<MarketDataSource> &marketDataSourceRef)
			: m_context(context),
			m_conf(conf),
			m_settings(settings),
			m_tradeSystem(tradeSystemRef),
			m_marketDataSource(marketDataSourceRef) {
		//...//
	}

public:

	void Bootstrap() {
		Assert(!m_tradeSystem);
		Assert(!m_marketDataSource);
		LoadTradeSystem();
		if (!m_marketDataSource) {
			LoadMarketDataSource();
		}
		Assert(m_tradeSystem);
		Assert(m_marketDataSource);
	}

private:

	void LoadTradeSystem() {
		
		Assert(!m_tradeSystem);
		Assert(!m_marketDataSource);
		
		const IniFileSectionRef configurationSection(
			m_conf,
			Ini::Sections::tradeSystem);
		const std::string module
			= configurationSection.ReadKey(Ini::Keys::module);
		std::string factoryName = configurationSection.ReadKey(
			Ini::Keys::factory,
			Ini::DefaultValues::Factories::tradeSystem);
		
		boost::shared_ptr<Dll> dll(new Dll(module, true));

		typedef TradeSystemFactoryResult FactoryResult;
		typedef TradeSystemFactory Factory;
		FactoryResult factoryResult;
		
		try {
			
			try {
				factoryResult = dll->GetFunction<Factory>(factoryName)(
					configurationSection,
					m_context.GetLog());
			} catch (const Dll::DllFuncException &) {
				if (	!boost::istarts_with(
							factoryName,
							Ini::DefaultValues::Factories::factoryNameStart)) {
					factoryName
						= Ini::DefaultValues::Factories::factoryNameStart
							+ factoryName;
					factoryResult = dll->GetFunction<Factory>(factoryName)(
						configurationSection,
						m_context.GetLog());
				} else {
					throw;
				}
			}
		
		} catch (...) {
			trdk::Log::RegisterUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__,
				false);
			throw Exception("Failed to load trade system module");
		}
	
		Assert(boost::get<0>(factoryResult));
		if (!boost::get<0>(factoryResult)) {
			throw Exception(
				"Failed to load trade system module - no object returned");
		}
		m_tradeSystem = DllObjectPtr<TradeSystem>(
			dll,
			boost::get<0>(factoryResult));

		if (boost::get<1>(factoryResult)) {
			m_context.GetLog().Debug(
				"Using trade system as market data source.");
			m_marketDataSource = DllObjectPtr<MarketDataSource>(
				dll,
				boost::get<1>(factoryResult));
		}

	}

	void LoadMarketDataSource() {
		
		Assert(!m_marketDataSource);

		const IniFileSectionRef configurationSection(
			m_conf,
			Ini::Sections::marketDataSource);
		const std::string module
			= configurationSection.ReadKey(Ini::Keys::module);
		std::string factoryName = configurationSection.ReadKey(
			Ini::Keys::factory,
			Ini::DefaultValues::Factories::marketDataSource);
		
		boost::shared_ptr<Dll> dll(new Dll(module, true));

		typedef boost::shared_ptr<MarketDataSource> FactoryResult;
		typedef MarketDataSourceFactory Factory;
		FactoryResult factoryResult;
		
		try {
		
			try {
				factoryResult = dll->GetFunction<Factory>(factoryName)(
					configurationSection);
			} catch (const Dll::DllFuncException &) {
				if (	!boost::istarts_with(
							factoryName,
							Ini::DefaultValues::Factories::factoryNameStart)) {
					factoryName
						= Ini::DefaultValues::Factories::factoryNameStart
							+ factoryName;
					factoryResult = dll->GetFunction<Factory>(factoryName)(
						configurationSection);
				} else {
					throw;
				}
			}
	
		} catch (...) {
			trdk::Log::RegisterUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__,
				false);
			throw Exception("Failed to load market data source module");
		}
	
		Assert(factoryResult);
		if (!factoryResult) {
			throw Exception(
				"Failed to load market data source module - no object returned");
		}
		m_marketDataSource = DllObjectPtr<MarketDataSource>(dll, factoryResult);

	}

private:

	Engine::Context &m_context;

	const IniFile &m_conf;
	const Settings &m_settings;

	DllObjectPtr<TradeSystem> &m_tradeSystem;
	DllObjectPtr<MarketDataSource> &m_marketDataSource;

};

////////////////////////////////////////////////////////////////////////////////

namespace {

	enum SystemService {
		SYSTEM_SERVICE_LEVEL1_UPDATES,
		SYSTEM_SERVICE_LEVEL1_TICKS,
		SYSTEM_SERVICE_TRADES,
		numberOfSystemServices
	};

	//! Module type.
	/** The order is important, it affects the boot priority. Services must be
	  * checked last - service can be required by other, at last stage it
	  * binds with systems service "by current strategy".
	  */
	enum ModuleType {
		MODULE_TYPE_STRATEGY,
		MODULE_TYPE_OBSERVER,
		MODULE_TYPE_SERVICE,
		numberOfModuleTypes
	};

	template<typename Module>
	struct ModuleTrait {
		//...//
	};
	template<>
	struct ModuleTrait<Strategy> {
		enum {
			Type = MODULE_TYPE_STRATEGY
		};
		typedef boost::shared_ptr<Strategy> (Factory)(
				trdk::Context &,
				const std::string &tag,
				const IniFileSectionRef &);
		static ModuleType GetType() {
			return static_cast<ModuleType>(Type);
		}
		static const char * GetName(bool capital = false) {
			return capital ? "Strategy" : "strategy";
		}
		static const std::string & GetDefaultFactory() {
			return Ini::DefaultValues::Factories::strategy;
		}
		static std::string GetDefaultModule() {
			return std::string();
		}
	};
	template<>
	struct ModuleTrait<Service> {
		enum {
			Type = MODULE_TYPE_SERVICE
		};
		typedef boost::shared_ptr<Service> (Factory)(
			trdk::Context &,
			const std::string &tag,
			const IniFileSectionRef &);
		static ModuleType GetType() {
			return static_cast<ModuleType>(Type);
		}
		static const char * GetName(bool capital = false) {
			return capital ? "Service" : "service";
		}
		static const std::string & GetDefaultFactory() {
			return Ini::DefaultValues::Factories::service;
		}
		static const std::string & GetDefaultModule() {
			return Ini::DefaultValues::Modules::service;
		}
	};
	template<>
	struct ModuleTrait<Observer> {
		enum {
			Type = MODULE_TYPE_OBSERVER
		};
		typedef boost::shared_ptr<Observer> (Factory)(
			trdk::Context &,
			const std::string &tag,
			const IniFileSectionRef &);
		static ModuleType GetType() {
			return static_cast<ModuleType>(Type);
		}
		static const char * GetName(bool capital = false) {
			return capital ? "Observer" : "observer";
		}
		static const std::string & GetDefaultFactory() {
			return Ini::DefaultValues::Factories::observer;
		}
		static std::string GetDefaultModule() {
			return std::string();
		}
	};

	struct TagRequirementsList {
		
		ModuleType subscriberType;
		std::string subscriberTag;
	
		std::map<
				std::string /* tag */,
				std::set<std::set<Symbol>>>
			requiredModules;

		std::map<
				SystemService,
				std::set<Symbol>>
			requiredSystemServices;

	};

	struct BySubscriber {
		//...//
	};

	typedef boost::multi_index_container<
			TagRequirementsList,
			mi::indexed_by<
				mi::ordered_unique<
					mi::tag<BySubscriber>,
					mi::composite_key<
						TagRequirementsList,
						mi::member<
							TagRequirementsList,
							ModuleType,
							// Order is important, @sa ModuleType
							&TagRequirementsList::subscriberType>,
						mi::member<
							TagRequirementsList,
							std::string,
							&TagRequirementsList::subscriberTag>>>>>
		RequirementsList;

	void ApplyRequirementsListModifier(
				ModuleType type,
				const std::string &tag,
				const boost::function<void (TagRequirementsList &)> &modifier,
				RequirementsList &list) {
		auto &index = list.get<BySubscriber>();
		auto pos = index.find(boost::make_tuple(type, tag));
		if (pos != index.end()) {
			Verify(index.modify(pos, modifier));
		} else {
			TagRequirementsList requirements = {};
			requirements.subscriberType = type;
			requirements.subscriberTag = tag;
			modifier(requirements);
			list.insert(requirements);
		}
	}

	struct SupplierRequest {
		std::string tag;
		std::set<Symbol> symbols;
	};

	template<typename Module>
	struct ModuleDll {
		boost::shared_ptr<IniFileSectionRef> conf;
		boost::shared_ptr<Lib::Dll> dll;
		typename ModuleTrait<Module>::Factory *factory;
		std::map<
				std::set<Symbol>,
				DllObjectPtr<Module>>
			symbolInstances;
		std::list<DllObjectPtr<Module>> standaloneInstances;
	};

	typedef std::map<std::string /*tag*/, ModuleDll<Strategy>> StrategyModules;
	typedef std::map<std::string /*tag*/, ModuleDll<Observer>> ObserverModules;
	typedef std::map<std::string /*tag*/, ModuleDll<Service>> ServiceModules;

	Symbol GetMagicSymbolCurrentSecurity() {
		return Symbol("$", "$", "$");
	}

	bool IsMagicSymbolCurrentSecurity(const Symbol &symbol) {
		Assert(
			(symbol.GetSymbol() == "$")
			== (symbol.GetExchange() == "$")
			== (symbol.GetPrimaryExchange() == "$"));
		return
			symbol.GetSymbol() == "$"
			&& symbol.GetExchange() == "$"
			&& symbol.GetPrimaryExchange() == "$";
	}

}

////////////////////////////////////////////////////////////////////////////////

class ContextStateBootstraper : private boost::noncopyable {

public:
	
	ContextStateBootstraper(
				const IniFile &confRef,
				Engine::Context &context,
				SubscriptionsManager &subscriptionsManagerRef,
				Strategies &strategiesRef,
				Observers &observersRef,
				Services &servicesRef)
			: m_context(context),
			m_subscriptionsManager(subscriptionsManagerRef),
			m_strategiesResult(strategiesRef),
			m_observersResult(observersRef),
			m_servicesResult(servicesRef),
			m_conf(confRef) {
		//...//
	}

public:

	void Bootstrap() {

		m_strategies.clear();
		m_observers.clear();
		m_services.clear();

		const auto sections = m_conf.ReadSectionsList();

		RequirementsList requirementList;
		foreach (const auto &sectionName, sections) {
			std::string type;
			std::string tag;
			std::unique_ptr<const IniFileSectionRef> section(
				GetModuleSection(m_conf, sectionName, type, tag));
			if (!section) {
				continue;
			}
			void (ContextStateBootstraper::*initModule)(
					const IniFileSectionRef &,
					const std::string &,
					RequirementsList &)
				= nullptr;
			if (boost::iequals(type, Ini::Sections::strategy)) {
				initModule = &ContextStateBootstraper::InitStrategy;
			} else if (boost::iequals(type, Ini::Sections::observer)) {
				initModule = &ContextStateBootstraper::InitObserver;
			} else if (boost::iequals(type, Ini::Sections::service)) {
				initModule = &ContextStateBootstraper::InitService;
			} else {
				AssertFail("Unknown module type");
				continue;
			}
			try {
				(this->*initModule)(*section, tag, requirementList);
			} catch (const Exception &ex) {
				m_context.GetLog().Error(
					"Failed to load module from \"%1%\": \"%2%\".",
					boost::make_tuple(
						boost::cref(*section),
						boost::cref(ex)));
				throw Exception("Failed to load module");
			}
		}

		if (m_strategies.empty()) {
			throw Exception("No strategies loaded");
		}

		try {
			BindWithRequirements(requirementList);
		} catch (...) {
			trdk::Log::RegisterUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__,
				false);
			throw Exception("Failed to load trade system module");
		}

		MakeModulesResult(m_strategies, m_strategiesResult);
		MakeModulesResult(m_observers, m_observersResult);
		MakeModulesResult(m_services, m_servicesResult);

		m_strategies.clear();
		m_observers.clear();
		m_services.clear();

	}

private:

	////////////////////////////////////////////////////////////////////////////////

	template<typename Module>
	void MakeModulesResult(
				const std::map<std::string /*tag*/, ModuleDll<Module>> &source,
				std::map<std::string /*tag*/, std::list<DllObjectPtr<Module>>> &result) {
		foreach (const auto &module, source) {
			const std::string &tag = module.first;
			const ModuleDll<Module> &moduleDll = module.second;
			auto &resultTag = result[tag];
			foreach (const auto &instance, moduleDll.symbolInstances) {
				resultTag.push_back(instance.second);
			}
			foreach (const auto &instance, moduleDll.standaloneInstances) {
				resultTag.push_back(instance);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////

	void InitStrategy(
				const IniFileSectionRef &section,
				const std::string &tag,
				RequirementsList &requirementList) {
		InitModule(section, tag, m_strategies, requirementList);
	}

	void InitObserver(
				const IniFileSectionRef &section,
				const std::string &tag,
				RequirementsList &requirementList) {
		InitModule(section, tag, m_observers, requirementList);
	}

	void InitService(
				const IniFileSectionRef &section,
				const std::string &tag,
				RequirementsList &requirementList) {
		InitModule(section, tag, m_services, requirementList);
	}

	template<typename Module>
	void InitModule(
				const IniFileSectionRef &conf,
				const std::string &tag,
				std::map<std::string /* tag */, ModuleDll<Module>> &modules,
				RequirementsList &requirementList) {

		typedef ModuleTrait<Module> Trait;

		m_context.GetLog().Debug(
			"Found %1% section \"%2%\"...",
			boost::make_tuple(
				boost::cref(Trait::GetName()),
				boost::cref(conf)));

		if (	boost::iequals(tag, Ini::Constants::Services::level1Updates)
				|| boost::iequals(tag, Ini::Constants::Services::level1Ticks)
				|| boost::iequals(tag, Ini::Constants::Services::trades)) {
			m_context.GetLog().Error(
				"System predefined module name used in %1%: \"%2%\".",
				boost::make_tuple(boost::cref(conf), boost::cref(tag)));
			throw Exception("System predefined module name used");
		}

		if (modules.find(tag) != modules.end()) {
			m_context.GetLog().Error(
				"Tag name \"%1%\" for %2% isn't unique.",
				boost::make_tuple(
					boost::cref(tag),
					boost::cref(Trait::GetName())));
			throw Exception("Tag name isn't unique");
		}

		ModuleDll<Module> module = LoadModuleDll<Module>(conf, tag);

		const std::set<Symbol> symbolInstances
			= GetSymbolInstances(Trait(), tag, *module.conf);
		if (!symbolInstances.empty()) {
			if (conf.ReadBoolKey(Ini::Keys::isStandalone, false)) {
				m_context.GetLog().Error(
					"Setting %1% provide instance list, but setting %2% sets"
						" %3% as \"standalone\".",
					boost::make_tuple(
						boost::cref(Ini::Keys::instances),
						boost::cref(Ini::Keys::isStandalone),
						boost::cref(Trait::GetName())));
				throw Exception("Settings conflict");
			}
			foreach (const auto &symbol, symbolInstances) {
				boost::shared_ptr<Module> instance
					= CreateModuleInstance(tag, symbol, module);
			}
		} else {
			CreateStandaloneModuleInstance(tag, module);
		}

		std::map<std::string , ModuleDll<Module>> modulesTmp(modules);
		modulesTmp[tag] = module;
		
		ReadRequirementsList<Module>(*module.conf, tag, requirementList);
		
		modulesTmp.swap(modules);

	}

	////////////////////////////////////////////////////////////////////////////////

	template<typename Module>
	boost::shared_ptr<Module> CreateModuleInstance(
				const std::string &tag,
				const std::set<Symbol> &symbols,
				ModuleDll<Module> &module) {
		typedef ModuleTrait<Module> Trait;
		boost::shared_ptr<Module> instance;
		try {
			instance = module.factory(m_context, tag, *module.conf);
		} catch (...) {
			trdk::Log::RegisterUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__,
				false);
			throw Exception("Failed to create module instance");
		}
		AssertEq(tag, instance->GetTag());
		DllObjectPtr<Module> instanceDllPtr(module.dll, instance);
		if (!symbols.empty()) {
			std::list<std::string> securities;
			foreach (auto symbol, symbols) {
				auto &security = LoadSecurity(symbol);
				try {
					instance->RegisterSource(security);
				} catch (...) {
					trdk::Log::RegisterUnhandledException(
						__FUNCTION__,
						__FILE__,
						__LINE__,
						false);
					throw Exception("Failed to attach security");
				}
				std::ostringstream oss;
				oss << security;
				securities.push_back(oss.str());
			}
			Assert(
				module.symbolInstances.find(symbols)
				== module.symbolInstances.end());
			module.symbolInstances[symbols] = instanceDllPtr;
			instance->GetLog().Info(
				"Instantiated for security(ies) \"%1%\".",
				boost::join(securities, ", "));
		} else {
			module.standaloneInstances.push_back(instanceDllPtr);
			instance->GetLog().Info("Instantiated.");
		}
		return instance;
	}

	template<typename Module>
	boost::shared_ptr<Module> CreateModuleInstance(
				const std::string &tag,
				const Symbol &symbol,
				ModuleDll<Module> &module) {
		std::set<Symbol> symbolSet;
		symbolSet.insert(symbol);
		return CreateModuleInstance(tag, symbolSet, module);
	}

	template<typename Module>
	boost::shared_ptr<Module> CreateModuleInstance(
				const std::string &tag,
				ModuleDll<Module> &module) {
		return CreateModuleInstance(tag, std::set<Symbol>(), module);
	}

	template<typename Module>
	void CreateStandaloneModuleInstance(
				const std::string &tag,
				ModuleDll<Module> &module) {
		typedef ModuleTrait<Module> Trait;
		static_assert(
			Trait::Type != MODULE_TYPE_SERVICE,
			"Wrong CreateStandaloneModuleInstance method choose.");
		if (module.conf->ReadBoolKey(Ini::Keys::isStandalone, false)) {
			m_context.GetLog().Error(
				"%1% can not be \"not standalone\" without \"%2%\".",
				boost::make_tuple(
					boost::cref(Trait::GetName(true)),
					Ini::Keys::requires));
			throw Exception("Settings conflict");
		}
		CreateModuleInstance(tag, module);
	}

	template<>
	void CreateStandaloneModuleInstance(
				const std::string &tag,
				ModuleDll<Service> &module) {
		typedef ModuleTrait<Service> Trait;
		if (module.conf->ReadBoolKey(Ini::Keys::isStandalone)) {
			CreateModuleInstance(tag, module);
		} else {
			m_context.GetLog().Debug(
				"%1% \"%2%\" instantiation delayed.",
				boost::make_tuple(
					boost::cref(Trait::GetName(true)),
					boost::cref(tag)));
		}
	}

	////////////////////////////////////////////////////////////////////////////////

	bool GetModuleSection(
				const std::string &sectionName,
				std::string &typeResult,
				std::string &tagResult)
			const {
		std::list<std::string> subs;
		boost::split(subs, sectionName, boost::is_any_of("."));
		if (subs.empty()) {
			return false;
		} else if (
				!boost::iequals(*subs.begin(), Ini::Sections::strategy)
				&& !boost::iequals(*subs.begin(), Ini::Sections::observer)
				&& !boost::iequals(*subs.begin(), Ini::Sections::service)) {
			return false;
		} else if (subs.size() != 2 || subs.rbegin()->empty()) {
			boost::format message(
				"Wrong module section name format: \"%1%\", %2%");
			message % sectionName;
			if (subs.size() < 2 || subs.rbegin()->empty()) {
				message % "expected tag name after dot";
			} else {
				message % "no extra dots allowed";
			}
			throw Exception(message.str().c_str());
		}
		subs.begin()->swap(typeResult);
		subs.rbegin()->swap(tagResult);
		return true;
	}

	//! Returns nullptr at fail.
	const IniFileSectionRef * GetModuleSection(
				const IniFile &ini,
				const std::string &sectionName,
				std::string &typeResult,
				std::string &tagResult)
			const {
		return GetModuleSection(sectionName, typeResult, tagResult)
			?	new IniFileSectionRef(ini, sectionName)
			:	nullptr;
	}

	////////////////////////////////////////////////////////////////////////////////

	SupplierRequest ParseSupplierRequest(const std::string &request) {

		SupplierRequest result;

		boost::smatch match;
		if (	!boost::regex_match(
					request,
					match,
					boost::regex("([^\\[]+)(\\[([^\\]]+)\\])?"))) {
			m_context.GetLog().Error(
				"Requirements syntax error: \"%1%\".",
				request);
			throw Exception("Requirements syntax error");
		}

		result.tag = boost::trim_copy(match[1].str());
		if (result.tag.empty()) {
			m_context.GetLog().Error(
				"Requirements syntax error: empty requirement tag in \"%1%\".",
				request);
			throw Exception("Requirements syntax error");
		}

		const std::string &symbolList = boost::trim_copy(match[3].str());
		if (symbolList.empty()) {
			result.symbols.insert(GetMagicSymbolCurrentSecurity());
			return result;
		}

		typedef boost::split_iterator<std::string::const_iterator> It;
		for (	It i = boost::make_split_iterator(
					symbolList,
					boost::first_finder(",", boost::is_iequal()))
				; i != It()
				; ++i) {
			const std::string symbolRequest
				= boost::copy_range<std::string>(*i);
			Symbol symbol = Symbol::Parse(
				symbolRequest,
				m_context.GetSettings().GetDefaultExchange(),
				m_context.GetSettings().GetDefaultPrimaryExchange());
			if (result.symbols.find(symbol) != result.symbols.end()) {
				m_context.GetLog().Error(
					"Requirements syntax error:"
						" duplicate entry \"%1%\" in \"%2%\".",
						boost::make_tuple(
							boost::cref(symbol),
							boost::cref(request)));
				throw Exception("Requirements syntax error");
			}
			result.symbols.insert(symbol);
		}

		return result;

	}

	template<typename Module>
	void ReadRequirementsList(
				const IniFileSectionRef &conf,
				const std::string &tag,
 				RequirementsList &result) {
		
		typedef ModuleTrait<Module> Trait;

		const std::string strList
			= conf.ReadKey(Ini::Keys::requires, std::string());
		std::list<std::string> list;
		bool isSymbolListOpened = false;
		typedef boost::split_iterator<std::string::const_iterator> It;
		for (	It i = boost::make_split_iterator(
					strList,
					boost::first_finder(",", boost::is_iequal()))
				; i != It()
				; ++i) {
			bool isNewRequirement = !isSymbolListOpened;
			if (!isSymbolListOpened) {
 				isSymbolListOpened
 					= std::find(boost::begin(*i), boost::end(*i), '[')
 						!= boost::end(*i);
			}
			if (
					isSymbolListOpened
					&& std::find(boost::begin(*i), boost::end(*i), ']')
							!= boost::end(*i)) {
				isSymbolListOpened = false;
			}
			if (isNewRequirement || list.empty()) {
				list.push_back(boost::copy_range<std::string>(*i));
			} else {
				list.rbegin()->push_back(',');
				list.rbegin()->insert(
					list.rbegin()->end(),
					boost::begin(*i),
					boost::end(*i));
			}
		}
		if (isSymbolListOpened) {
			m_context.GetLog().Error(
				"Requirements syntax error:"
					" expected closing \"]\" for \"%1%\" in \"%2%\".",
					boost::make_tuple(
						boost::cref(*list.rbegin()),
						boost::cref(strList)));
			throw Exception("Requirements syntax error");
		}

		foreach (std::string &request, list) {
			boost::trim(request);
			if (	boost::iequals(
						request,
						Ini::Constants::Services::level1Updates)) {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					SYSTEM_SERVICE_LEVEL1_UPDATES,
					ParseSupplierRequest(request),
					result);
			} else if (	boost::iequals(
							request,
							Ini::Constants::Services::level1Ticks)) {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					SYSTEM_SERVICE_LEVEL1_TICKS,
					ParseSupplierRequest(request),
					result);
			} else if (
					boost::iequals(
						request,
						Ini::Constants::Services::trades)) {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					SYSTEM_SERVICE_TRADES,
					ParseSupplierRequest(request),
					result);
			} else {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					ParseSupplierRequest(request),
					result);
			}
		}

	}

	void UpdateRequirementsList(
				ModuleType type,
				const std::string &tag,
				SystemService requiredService,
				const SupplierRequest &supplierRequest,
				RequirementsList &list) {
		ApplyRequirementsListModifier(
			type,
			tag,
			[&](TagRequirementsList &requirements) {
				requirements.requiredSystemServices[requiredService].insert(
					supplierRequest.symbols.begin(),
					supplierRequest.symbols.end());
			},
			list);
	}

	void UpdateRequirementsList(
				ModuleType type,
				const std::string &tag,
				const SupplierRequest &supplierRequest,
				RequirementsList &list) {
		ApplyRequirementsListModifier(
			type,
			tag,
			[&](TagRequirementsList &requirements) {
				auto &module = requirements.requiredModules[supplierRequest.tag];
				module.insert(supplierRequest.symbols);
			},
			list);
	}

	void BindWithRequirements(const RequirementsList &requirements) {
		foreach (
				const TagRequirementsList &moduleRequirements,
				requirements.get<BySubscriber>()) {
			static_assert(
				numberOfModuleTypes == 3,
				"Changed module type list.");
			switch (moduleRequirements.subscriberType) {
				case MODULE_TYPE_STRATEGY:
					BindModuleWithRequirements<Strategy>(
						moduleRequirements,
						m_strategies);
					break;
				case MODULE_TYPE_SERVICE:
					BindModuleWithRequirements<Service>(
						moduleRequirements,
						m_services);
					break;
				case MODULE_TYPE_OBSERVER:
					BindModuleWithRequirements<Observer>(
						moduleRequirements,
						m_observers);
					break;
				default:
					AssertEq(
						MODULE_TYPE_STRATEGY,
						moduleRequirements.subscriberType);
					break;
			}
		}
	}

	template<typename Module>
	void BindModuleWithRequirements(
				const TagRequirementsList &requirements,
				std::map<std::string /*tag*/, ModuleDll<Module>> &modules) {

		typedef ModuleTrait<Module> Trait;
		AssertEq(Trait::Type, requirements.subscriberType);
		const auto modulePos = modules.find(requirements.subscriberTag);
		Assert(modulePos != modules.end());
		if (modulePos == modules.end()) {
			return;
		}
		ModuleDll<Module> &module = modulePos->second;

		foreach (
				const auto &requirement,
				requirements.requiredSystemServices) {
			void (SubscriptionsManager::*subscribe)(Security &, Module &)
				= nullptr;
			static_assert(
				numberOfSystemServices == 3,
				"System service list changed.");
			switch (requirement.first) {
				case SYSTEM_SERVICE_LEVEL1_UPDATES:
					subscribe
						= &SubscriptionsManager::SubscribeToLevel1Updates;
					break;
				case SYSTEM_SERVICE_LEVEL1_TICKS:
					subscribe
						= &SubscriptionsManager::SubscribeToLevel1Ticks;
					break;
				case SYSTEM_SERVICE_TRADES:
					subscribe
						= &SubscriptionsManager::SubscribeToLevel1Ticks;
					break;
				default:
					AssertEq(SYSTEM_SERVICE_LEVEL1_UPDATES, requirement.first);
					break;
			}
			if (!subscribe) {
				continue;
			}
			foreach (const Symbol &symbol, requirement.second) {
				Security *security = nullptr;
				if (!IsMagicSymbolCurrentSecurity(symbol)) {
					security = &LoadSecurity(symbol);
				}
				foreach (auto &instance, module.standaloneInstances) {
					if (security) {
						(m_subscriptionsManager.*subscribe)(
							*security,
							*instance);
						instance->RegisterSource(*security);
					} else {
						SubscribeForEachSubscribedSecurity(
							*instance,
							[&](Security &subscribedSecurity) {
								(m_subscriptionsManager.*subscribe)(
									subscribedSecurity,
									*instance);
							});
					}
				}
				foreach (auto &instance, module.symbolInstances) {
					if (security) {
						(m_subscriptionsManager.*subscribe)(
							*security,
							*instance.second);
						instance.second->RegisterSource(*security);
					} else {
						SubscribeForEachSubscribedSecurity(
							*instance.second,
							[&](Security &subscribedSecurity) {
								(m_subscriptionsManager.*subscribe)(
									subscribedSecurity,
									*instance.second);
							});
					}
				}
			}
		}
		
		foreach (const auto &requirement, requirements.requiredModules) {
			const auto &requirementTag = requirement.first;
			const auto requredModulePos = m_services.find(requirementTag);
			Assert(requredModulePos != m_services.end());
			if (requredModulePos == m_services.end()) {
				continue;
			}
 			ModuleDll<Service> &requredModule = requredModulePos->second;
 			foreach (const std::set<Symbol> &symbols, requirement.second) {
				std::list<std::string> symbolsStr;
				foreach (const Symbol &symbol, symbols) {
					symbolsStr.push_back(symbol.GetAsString());
				}
				const std::string symbolsStrList
					= boost::join(symbolsStr, ", ");
 				const auto requredServicePos
 					= requredModule.symbolInstances.find(symbols);
				boost::shared_ptr<Service> requredService;
 				if (requredServicePos != requredModule.symbolInstances.end()) {
					requredService = requredServicePos->second.GetObjPtr();
				} else {
					requredService = CreateModuleInstance(
						requirementTag,
						symbols,
						requredModule);
					Assert(
						requredModule.symbolInstances.find(symbols)
						!= requredModule.symbolInstances.end());
 				}
				foreach (auto &instance, module.standaloneInstances) {
					requredService->RegisterSubscriber(*instance);
					instance->GetLog().Info(
						"Subscribed to \"%1%\" with security(ies) \"%2%\".",
						boost::make_tuple(
							boost::cref(*requredService),
							boost::cref(symbolsStrList)));
				}
				foreach (auto &instance, module.symbolInstances) {
					requredService->RegisterSubscriber(*instance.second);
					instance.second->GetLog().Info(
						"Subscribed to \"%1%\" with security(ies) \"%2%\".",
						boost::make_tuple(
							boost::cref(*requredService),
							boost::cref(symbolsStrList)));
				}
				
 			}
		}

	}

	template<typename Module, typename Pred>
	void SubscribeForEachSubscribedSecurity(
				Module &module,
				const Pred &pred) {
		const auto begin = module.GetSecurities().GetBegin();
		const auto end = module.GetSecurities().GetEnd();
		for (auto i = begin; i != end; ++i) {
			pred(*i);
		}
	}
	template<typename Pred>
	void SubscribeForEachSubscribedSecurity(
				Service &module,
				const Pred &pred) {
		const auto begin = module.GetSecurities().GetBegin();
		const auto end = module.GetSecurities().GetEnd();
		for (auto i = begin; i != end; ++i) {
			Assert(
				m_context.GetMarketDataSource().FindSecurity(i->GetSymbol()));
			pred(LoadSecurity(i->GetSymbol()));
		}
	}

	////////////////////////////////////////////////////////////////////////////////

	Security & LoadSecurity(const Symbol &symbol) {
		return m_context.GetMarketDataSource().GetSecurity(m_context, symbol);
	}

	////////////////////////////////////////////////////////////////////////////////

	template<typename Module>
	std::set<Symbol> GetSymbolInstances(
				const ModuleTrait<Module> &,
				const std::string &tag,
				const IniFileSectionRef &conf) {

		typedef ModuleTrait<Module> Trait;
		static_assert(
			Trait::Type != MODULE_TYPE_OBSERVER,
			"Wrong GetSymbolInstances method choose.");

		std::set<Symbol> result;

		if (!conf.IsKeyExist(Ini::Keys::instances)) {
			return result;
		}

		fs::path symbolsFilePath;
		try {
			symbolsFilePath = Normalize(
				conf.ReadKey(Ini::Keys::instances),
				conf.GetBase().GetPath().branch_path());
		} catch (const IniFile::Error &ex) {
			m_context.GetLog().Error(
				"Failed to get symbol instances file: \"%1%\".",
				ex);
			throw;
		}
		m_context.GetLog().Debug(
			"Loading symbol instances from %1% for %2% \"%3%\"...",
			boost::make_tuple(
				boost::cref(symbolsFilePath),
				boost::cref(Trait::GetName()),
				boost::cref(tag)));
		const IniFile symbolsIni(symbolsFilePath);
		std::set<Symbol> symbols = symbolsIni.ReadSymbols(
			m_context.GetSettings().GetDefaultExchange(),
			m_context.GetSettings().GetDefaultPrimaryExchange());

		try {
			foreach (const auto &iniSymbol, symbols) {
				result.insert(Symbol(iniSymbol));
			}
		} catch (const IniFile::Error &ex) {
			m_context.GetLog().Error(
				"Failed to load securities for %2%: \"%1%\".",
				boost::make_tuple(
					boost::cref(ex),
					boost::cref(tag)));
			throw IniFile::Error("Failed to load securities");
		}
	
		return result;

	}

	template<>
	std::set<Symbol> GetSymbolInstances(
				const ModuleTrait<Observer> &,
				const std::string &,
				const IniFileSectionRef &) {
		return std::set<Symbol>();
	}

	////////////////////////////////////////////////////////////////////////////////

	template<typename Module>
	ModuleDll<Module> LoadModuleDll(
				const IniFileSectionRef &conf,
				const std::string &tag)
			const {

		typedef ModuleTrait<Module> Trait;

		ModuleDll<Module> result;
		result.conf.reset(new IniFileSectionRef(conf));

		if (tag.empty()) {
			m_context.GetLog().Error(
				"Failed to get tag for %1% section \"%2%\".",
				boost::make_tuple(
					boost::cref(Trait::GetName()),
					boost::cref(result.conf)));
			throw IniFile::Error("Failed to load module");
		}

		fs::path modulePath;
		try {
			if (	!result.conf->IsKeyExist(Ini::Keys::module)
					&& !Trait::GetDefaultModule().empty()) {
				modulePath = Normalize(Trait::GetDefaultModule());
			} else {
				modulePath = result.conf->ReadFileSystemPath(Ini::Keys::module);
			}
		} catch (const IniFile::Error &ex) {
			m_context.GetLog().Error(
				"Failed to get %1% module: \"%2%\".",
				boost::make_tuple(
					boost::cref(Trait::GetName()),
					boost::cref(ex)));
			throw IniFile::Error("Failed to load module");
		}

		result.dll.reset(new Dll(modulePath, true));

		const std::string factoryName = result.conf->ReadKey(
			Ini::Keys::factory,
			Trait::GetDefaultFactory());
		try {
			result.factory
				= result.dll->GetFunction<Trait::Factory>(factoryName);
		} catch (const Dll::DllFuncException &) {
			if (	boost::istarts_with(
						factoryName,
						Ini::DefaultValues::Factories::factoryNameStart)) {
				throw;
			}
			result.factory = result.dll->GetFunction<Trait::Factory>(
				Ini::DefaultValues::Factories::factoryNameStart + factoryName);
		}

		return result;

	}

	////////////////////////////////////////////////////////////////////////////////

private:

	Engine::Context &m_context;

	SubscriptionsManager &m_subscriptionsManager;

	Strategies &m_strategiesResult;
	Observers &m_observersResult;
	Services &m_servicesResult;

	StrategyModules m_strategies;
	ObserverModules m_observers;
	ServiceModules m_services;

	const IniFile &m_conf;

};

////////////////////////////////////////////////////////////////////////////////

void Engine::BootstrapContext(
			const IniFile &conf,
			const Settings &settings,
			Context &context,
			DllObjectPtr<TradeSystem> &tradeSystemRef,
			DllObjectPtr<MarketDataSource> &marketDataSourceRef) {
	ContextBootstraper(
			conf,
			settings,
			context,
			tradeSystemRef,
			marketDataSourceRef)
		.Bootstrap();
}

void Engine::BootstrapContextState(
			const IniFile &conf,
			Context &context,
			SubscriptionsManager &subscriptionsManagerRef,
			Strategies &strategiesRef,
			Observers &observersRef,
			Services &servicesRef) {
	ContextStateBootstraper(
			conf,
			context,
			subscriptionsManagerRef,
			strategiesRef,
			observersRef,
			servicesRef)
		.Bootstrap();
}

////////////////////////////////////////////////////////////////////////////////
