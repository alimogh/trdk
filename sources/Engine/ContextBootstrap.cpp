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
using namespace trdk::Engine::Ini;

////////////////////////////////////////////////////////////////////////////////

namespace {

	bool GetModuleSection(
				const std::string &sectionName,
				std::string &typeResult,
				std::string &tagResult) {
		std::list<std::string> subs;
		boost::split(subs, sectionName, boost::is_any_of("."));
		if (subs.empty()) {
			return false;
		} else if (
				!boost::iequals(*subs.begin(), Sections::strategy)
				&& !boost::iequals(*subs.begin(), Sections::observer)
				&& !boost::iequals(*subs.begin(), Sections::service)) {
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
	const IniSectionRef * GetModuleSection(
				const Lib::Ini &ini,
				const std::string &sectionName,
				std::string &typeResult,
				std::string &tagResult) {
		return GetModuleSection(sectionName, typeResult, tagResult)
			?	new IniSectionRef(ini, sectionName)
			:	nullptr;
	}

	bool GetMarketDataSourceSection(
				const std::string &sectionName,
				std::string &tagResult) {
		std::list<std::string> subs;
		boost::split(subs, sectionName, boost::is_any_of("."));
		if (subs.empty() || subs.size() == 1) {
			return false;
		} else if (!boost::iequals(*subs.begin(), Sections::marketDataSource)) {
			return false;
		} else if (
				(subs.size() == 2 && subs.rbegin()->empty())
				|| subs.size() != 2) {
			boost::format message(
				"Wrong Market Data Source section name format: \"%1%\", %2%");
			message % sectionName;
			if (subs.size() < 2 || subs.rbegin()->empty()) {
				message % "expected tag name after dot";
			} else {
				message % "no extra dots allowed";
			}
			throw Exception(message.str().c_str());
		}
		subs.rbegin()->swap(tagResult);
		return true;
	}

	bool GetTradeSystemSection(
				const std::string &sectionName,
				std::string &tagResult) {
		std::list<std::string> subs;
		boost::split(subs, sectionName, boost::is_any_of("."));
		if (subs.empty() || subs.size() == 1) {
			return false;
		} else if (!boost::iequals(*subs.begin(), Sections::tradeSystem)) {
			return false;
		} else if (
				(subs.size() == 2 && subs.rbegin()->empty())
				|| subs.size() != 2) {
			boost::format message(
				"Wrong Trade System section name format: \"%1%\", %2%");
			message % sectionName;
			if (subs.size() < 2 || subs.rbegin()->empty()) {
				message % "expected tag name after dot";
			} else {
				message % "no extra dots allowed";
			}
			throw Exception(message.str().c_str());
		}
		subs.rbegin()->swap(tagResult);
		return true;
	}

}

////////////////////////////////////////////////////////////////////////////////

class ContextBootstrapper : private boost::noncopyable {

public:

	explicit ContextBootstrapper(
				const Lib::Ini &conf,
				Engine::Context &context,
				TradeSystems &tradeSystemsRef,
				MarketDataSources &marketDataSourcesRef)
			: m_context(context),
			m_conf(conf),
			m_tradeSystems(tradeSystemsRef),
			m_marketDataSources(marketDataSourcesRef) {
		//...//
	}

public:

	void Boot() {
		LoadContextParams();
		AssertEq(0, m_tradeSystems.size());
		AssertEq(0, m_marketDataSources.size());
		LoadTradeSystems();
		AssertLt(0, m_tradeSystems.size());
		LoadMarketDataSources();
		AssertLt(0, m_tradeSystems.size());
		AssertLt(0, m_marketDataSources.size());
	}

private:

	void LoadContextParams() {
		const auto &pred = [&](
					const std::string &key,
					const std::string &name)
				-> bool {
			m_context.GetParams().Update(key, name);
			return true;
		};
		m_conf.ForEachKey(Sections::contextParams, pred, false);
	}

	//! Loads Market Data Source by conf. section name.
	void LoadTradeSystem(
			size_t index,
			const IniSectionRef &configurationSection,
			const std::string &tag,
			DllObjectPtr<TradeSystem> &tradeSystemResult,
			DllObjectPtr<MarketDataSource> &marketDataSourceResult) {
		
		const std::string module
			= configurationSection.ReadKey(Keys::module);
		std::string factoryName = configurationSection.ReadKey(
			Keys::factory,
			DefaultValues::Factories::tradeSystem);
		
		boost::shared_ptr<Dll> dll(
			new Dll(
				module,
				configurationSection.ReadBoolKey(Keys::Dbg::autoName, true)));

		typedef TradeSystemFactoryResult FactoryResult;
		typedef TradeSystemFactory Factory;
		FactoryResult factoryResult;
		
		try {
			
			try {
				factoryResult = dll->GetFunction<Factory>(factoryName)(
					index,
					m_context,
					tag,
					configurationSection);
			} catch (const Dll::DllFuncException &) {
				if (	!boost::istarts_with(
							factoryName,
							DefaultValues::Factories::factoryNameStart)) {
					factoryName
						= DefaultValues::Factories::factoryNameStart
							+ factoryName;
					factoryResult = dll->GetFunction<Factory>(factoryName)(
						index,
						m_context,
						tag,
						configurationSection);
				} else {
					throw;
				}
			}
		
		} catch (...) {
			trdk::EventsLog::BroadcastUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__);
			throw Exception("Failed to load trade system module");
		}
	
		Assert(boost::get<0>(factoryResult));
		if (!boost::get<0>(factoryResult)) {
			throw Exception(
				"Failed to load trade system module - no object returned");
		}

		tradeSystemResult = DllObjectPtr<TradeSystem>(
			dll,
			boost::get<0>(factoryResult));
		if (boost::get<1>(factoryResult)) {
			marketDataSourceResult = DllObjectPtr<MarketDataSource>(
				dll,
				boost::get<1>(factoryResult));
		}

	}

	//! Loads Trade Systems.
	/** Reads all sections from configuration, filters Trade Systems
	  * sections and loads service for it.
	  */
	void LoadTradeSystems() {
	
		foreach (const auto &section, m_conf.ReadSectionsList()) {
			
			std::string tag;
			if (	
					!GetTradeSystemSection(section, tag)
					&& !boost::iequals(section, Sections::tradeSystem)) {
				continue;
			}
			
			DllObjectPtr<TradeSystem> tradeSystem;
			DllObjectPtr<MarketDataSource> marketDataSource;
			LoadTradeSystem(
				m_tradeSystems.size(),
				IniSectionRef(m_conf, section),
				tag,
				tradeSystem,
				marketDataSource);

			// It always must be a trade system service...
			Assert(tradeSystem);
			TradeSystemHolder tradeSystemHolder;
			tradeSystemHolder.tradeSystem = tradeSystem;
			m_tradeSystems.push_back(tradeSystemHolder);

			// ...and can be Market Data Source at the same time:
			if (marketDataSource) {
				m_context.GetLog().Info(
					"Using Trade System as Market Data Source.");
				m_marketDataSources.push_back(marketDataSource);
			}

		}
	
		if (m_tradeSystems.empty()) {
			throw Exception("No one TradeSystem found in configuration");
		}

		m_tradeSystems.shrink_to_fit();
	
	}

	//! Loads Market Data Source by conf. section name.
	DllObjectPtr<MarketDataSource> LoadMarketDataSource(
			size_t index,
			const IniSectionRef &configurationSection,
			const std::string &tag) {
		
		const std::string module
			= configurationSection.ReadKey(Keys::module);
		std::string factoryName = configurationSection.ReadKey(
			Keys::factory,
			DefaultValues::Factories::marketDataSource);
		
		boost::shared_ptr<Dll> dll(
			new Dll(
				module,
				configurationSection.ReadBoolKey(Keys::Dbg::autoName, true)));

		typedef boost::shared_ptr<MarketDataSource> FactoryResult;
		typedef MarketDataSourceFactory Factory;
		FactoryResult factoryResult;
		
		try {
		
			try {
				factoryResult = dll->GetFunction<Factory>(factoryName)(
					index,
					m_context,
					tag,
					configurationSection);
			} catch (const Dll::DllFuncException &) {
				if (	!boost::istarts_with(
							factoryName,
							DefaultValues::Factories::factoryNameStart)) {
					factoryName
						= DefaultValues::Factories::factoryNameStart
							+ factoryName;
					factoryResult = dll->GetFunction<Factory>(factoryName)(
						index,
						m_context,
						tag,
						configurationSection);
				} else {
					throw;
				}
			}
	
		} catch (...) {
			trdk::EventsLog::BroadcastUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__);
			throw Exception("Failed to load market data source module");
		}
	
		Assert(factoryResult);
		if (!factoryResult) {
			throw Exception(
				"Failed to load market data source module - no object returned");
		}
		
		return DllObjectPtr<MarketDataSource>(dll, factoryResult);

	}

	//! Loads Market Data Sources.
	/** Reads all sections from configuration, filters Marker Data Source
	  * sections and loads service for it.
	  */
	void LoadMarketDataSources() {
	
		foreach (const auto &section, m_conf.ReadSectionsList()) {
			std::string tag;
			if (	!GetMarketDataSourceSection(section, tag)
					&& !boost::iequals(section, Sections::marketDataSource)) {
				continue;
			}
			const auto &source = LoadMarketDataSource(
				m_marketDataSources.size(),
				IniSectionRef(m_conf, section),
				tag);
			m_marketDataSources.push_back(source);
		}
	
		
		if (m_marketDataSources.empty()) {
			throw Exception("No one Market Data Source found in configuration");
		}

		m_marketDataSources.shrink_to_fit();
	
	}

private:

	Engine::Context &m_context;

	const Lib::Ini &m_conf;

	TradeSystems &m_tradeSystems;
	MarketDataSources &m_marketDataSources;

};

////////////////////////////////////////////////////////////////////////////////

namespace {

	enum SystemService {
		SYSTEM_SERVICE_LEVEL1_UPDATES,
		SYSTEM_SERVICE_LEVEL1_TICKS,
		SYSTEM_SERVICE_BOOK_UPDATE_TICKS,
		SYSTEM_SERVICE_TRADES,
		SYSTEM_SERVICE_BROKER_POSITIONS_UPDATES,
		SYSTEM_SERVICE_BARS,
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

	template<typename ModuleTrait>
	std::string BuildDefaultFactoryName(const std::string &tag) {
		return
			DefaultValues::Factories::factoryNameStart
			+ tag
			+ ModuleTrait::GetName(true);
	}

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
				const IniSectionRef &);
		static ModuleType GetType() {
			return static_cast<ModuleType>(Type);
		}
		static const char * GetName(bool capital) {
			return capital ? "Strategy" : "strategy";
		}
		static std::string GetDefaultFactory(const std::string &tag) {
			return BuildDefaultFactoryName<ModuleTrait>(tag);
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
			const IniSectionRef &);
		static ModuleType GetType() {
			return static_cast<ModuleType>(Type);
		}
		static const char * GetName(bool capital) {
			return capital ? "Service" : "service";
		}
		static std::string GetDefaultFactory(const std::string &tag) {
			return BuildDefaultFactoryName<ModuleTrait>(tag);
		}
		static const std::string & GetDefaultModule() {
			return DefaultValues::Modules::service;
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
			const IniSectionRef &);
		static ModuleType GetType() {
			return static_cast<ModuleType>(Type);
		}
		static const char * GetName(bool capital) {
			return capital ? "Observer" : "observer";
		}
		static std::string GetDefaultFactory(const std::string &tag) {
			return BuildDefaultFactoryName<ModuleTrait>(tag);
		}
		static std::string GetDefaultModule() {
			return std::string();
		}
	};

	struct TagRequirementsList {
		
		ModuleType subscriberType;
		std::string subscriberTag;
		Module *uniqueInstance;
	
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
							&TagRequirementsList::subscriberTag>,
						mi::member<
							TagRequirementsList,
							Module *,
							&TagRequirementsList::uniqueInstance>>>>>
		RequirementsList;

	void ApplyRequirementsListModifier(
				ModuleType type,
				const std::string &tag,
				Module *uniqueInstance,
				const boost::function<void (TagRequirementsList &)> &modifier,
				RequirementsList &list) {
		auto &index = list.get<BySubscriber>();
		auto pos = index.find(boost::make_tuple(type, tag, uniqueInstance));
		if (pos != index.end()) {
			Verify(index.modify(pos, modifier));
		} else {
			TagRequirementsList requirements = {};
			requirements.subscriberType = type;
			requirements.subscriberTag = tag;
			requirements.uniqueInstance = uniqueInstance;
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
		boost::shared_ptr<IniSectionRef> conf;
		boost::shared_ptr<Lib::Dll> dll;
		typename ModuleTrait<Module>::Factory *factory;
		//! Instances which have symbol sets.
		std::map<std::set<Symbol>, DllObjectPtr<Module>> symbolInstances;
		//! Instances that are created without symbols.
		std::list<DllObjectPtr<Module>> standaloneInstances;
	};

	template<
		typename Module,
		typename PredForStandalone,
		typename PredForSymbols>
	void ForEachModuleInstance(
				ModuleDll<Module> &module,
				const PredForStandalone &predForStandalone,
				const PredForSymbols &predForSymbols) {
		foreach (auto &instance, module.standaloneInstances) {
			predForStandalone(*instance);
		}
		foreach (auto &instance, module.symbolInstances) {
			predForSymbols(*instance.second);
		}
	}

	typedef std::map<std::string /*tag*/, ModuleDll<Strategy>> StrategyModules;
	typedef std::map<std::string /*tag*/, ModuleDll<Observer>> ObserverModules;
	typedef std::map<std::string /*tag*/, ModuleDll<Service>> ServiceModules;

}

////////////////////////////////////////////////////////////////////////////////

class ContextStateBootstrapper : private boost::noncopyable {

public:
	
	explicit ContextStateBootstrapper(
				const Lib::Ini &confRef,
				Engine::Context &context,
				SubscriptionsManager &subscriptionsManagerRef,
				Strategies &strategiesRef,
				Observers &observersRef,
				Services &servicesRef,
				ModuleList &moduleListRef)
			: m_context(context),
			m_subscriptionsManager(subscriptionsManagerRef),
			m_strategiesResult(&strategiesRef),
			m_observersResult(&observersRef),
			m_servicesResult(&servicesRef),
			m_moduleListResult(moduleListRef),
			m_conf(confRef) {
		//...//
	}

	explicit ContextStateBootstrapper(
				const Lib::Ini &confRef,
				Engine::Context &context,
				SubscriptionsManager &subscriptionsManagerRef,
				Strategies &strategiesRef,
				Services &servicesRef,
				ModuleList &moduleListRef)
			: m_context(context),
			m_subscriptionsManager(subscriptionsManagerRef),
			m_strategiesResult(&strategiesRef),
			m_observersResult(nullptr),
			m_servicesResult(&servicesRef),
			m_moduleListResult(moduleListRef),
			m_conf(confRef) {
		//...//
	}

public:

	void Boot() {

		m_strategies.clear();
		m_observers.clear();
		m_services.clear();

		const auto sections = m_conf.ReadSectionsList();

		RequirementsList requirementList;
		foreach (const auto &sectionName, sections) {
			std::string type;
			std::string tag;
			std::unique_ptr<const IniSectionRef> section(
				GetModuleSection(m_conf, sectionName, type, tag));
			if (!section) {
				continue;
			}
			void (ContextStateBootstrapper::*initModule)(
					const IniSectionRef &,
					const std::string &,
					RequirementsList &)
				= nullptr;
			if (boost::iequals(type, Sections::strategy)) {
				initModule = &ContextStateBootstrapper::InitStrategy;
			} else if (boost::iequals(type, Sections::observer)) {
				initModule = &ContextStateBootstrapper::InitObserver;
			} else if (boost::iequals(type, Sections::service)) {
				initModule = &ContextStateBootstrapper::InitService;
			} else {
				AssertFail("Unknown module type");
				continue;
			}
			try {
				(this->*initModule)(*section, tag, requirementList);
			} catch (const Exception &ex) {
				m_context.GetLog().Error(
					"Failed to load module from \"%1%\": \"%2%\".",
					*section,
					ex);
				throw Exception("Failed to load module");
			}
		}

		try {
			BindWithModuleRequirements(requirementList);
		} catch (...) {
			trdk::EventsLog::BroadcastUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__);
			throw Exception("Failed to build system modules relationship");
		}
		try {
			BindWithSystemRequirements(requirementList);
		} catch (...) {
			trdk::EventsLog::BroadcastUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__);
			throw Exception("Failed to build modules relationship");
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
				std::map<std::string /*tag*/, ModuleDll<Module>> &source,
				std::map<
						std::string /*tag*/,
						std::vector<boost::shared_ptr<Module>>> *
					result) {
		
		if (!result) {
			AssertEq(0, source.size());
			return;
		}
		
		foreach (auto &module, source) {
			const std::string &tag = module.first;
			ModuleDll<Module> &moduleDll = module.second;
			auto &resultTag = (*result)[tag];
			foreach (auto &instance, moduleDll.symbolInstances) {
				resultTag.push_back(instance.second);
			}
			foreach (auto &instance, moduleDll.standaloneInstances) {
				resultTag.push_back(instance);
			}
			m_moduleListResult.insert(moduleDll.dll);
		}
		
		foreach (auto &tag, *result) {
			tag.second.shrink_to_fit();
		}

	}

	////////////////////////////////////////////////////////////////////////////////

	void InitStrategy(
				const IniSectionRef &section,
				const std::string &tag,
				RequirementsList &requirementList) {
		if (!m_strategiesResult) {
			m_context.GetLog().Error(
				"Strategy section \"%1%\" is found"
					", but strategies can not be added.",
				section.GetName());
			throw Exception(
				"Strategy section is found, but strategies can not be added");
		}
		InitModule(section, tag, m_strategies, requirementList);
	}

	void InitObserver(
				const IniSectionRef &section,
				const std::string &tag,
				RequirementsList &requirementList) {
		if (!m_observersResult) {
			m_context.GetLog().Error(
				"Observer section \"%1%\" is found"
					", but strategies can not be added.",
				section.GetName());
			throw Exception(
				"Observer section is found, but strategies can not be added");
		}
		InitModule(section, tag, m_observers, requirementList);
	}

	void InitService(
				const IniSectionRef &section,
				const std::string &tag,
				RequirementsList &requirementList) {
		if (!m_servicesResult) {
			m_context.GetLog().Error(
				"Service section \"%1%\" is found"
					", but strategies can not be added.",
				section.GetName());
			throw Exception(
				"Service section is found, but strategies can not be added");
		}
		InitModule(section, tag, m_services, requirementList);
	}

	template<typename Module>
	void InitModule(
				const IniSectionRef &conf,
				const std::string &tag,
				std::map<std::string /* tag */, ModuleDll<Module>> &modules,
				RequirementsList &requirementList) {

		typedef ModuleTrait<Module> Trait;

		m_context.GetLog().Debug(
			"Found %1% section \"%2%\"...",
			Trait::GetName(false),
			conf);

		static_assert(
			numberOfSystemServices == 6,
			"System service list changed.");
		if (	boost::iequals(tag, Constants::Services::level1Updates)
				|| boost::iequals(tag, Constants::Services::level1Ticks)
				|| boost::iequals(tag, Constants::Services::bookUpdateTicks)
				|| boost::iequals(tag, Constants::Services::trades)
				|| boost::iequals(
						tag,
						Constants::Services::brokerPositionsUpdates)
				|| boost::iequals(tag, Constants::Services::bars)) {
			m_context.GetLog().Error(
				"System predefined module name used in %1%: \"%2%\".",
				conf, 
				tag);
			throw Exception("System predefined module name used");
		}

		if (modules.find(tag) != modules.end()) {
			m_context.GetLog().Error(
				"Tag name \"%1%\" for %2% isn't unique.",
				tag,
				false);
			throw Exception("Tag name isn't unique");
		}

		ModuleDll<Module> module = LoadModuleDll<Module>(conf, tag);

		const std::set<Symbol> symbolInstances
			= GetSymbolInstances(Trait(), tag, *module.conf);
		if (!symbolInstances.empty()) {
			foreach (const auto &symbol, symbolInstances) {
				CreateModuleInstance(tag, symbol, module);
			}
		} else {
			CreateStandaloneModuleInstance(tag, module);
		}

		std::map<std::string, ModuleDll<Module>> modulesTmp(modules);
		modulesTmp[tag] = module;
		
		ReadRequirementsList<Module>(module, tag, requirementList);

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
			trdk::EventsLog::BroadcastUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__);
			throw Exception("Failed to create module instance");
		}
		AssertEq(tag, instance->GetTag());
		DllObjectPtr<Module> instanceDllPtr(module.dll, instance);
		if (!symbols.empty()) {
			std::vector<std::string> securities;
			foreach (auto symbol, symbols) {
				Assert(symbol);
				m_context.ForEachMarketDataSource([&](
							MarketDataSource &source)
						-> bool {
					auto &security = source.GetSecurity(symbol);
					try {
						instance->RegisterSource(security);
					} catch (...) {
						trdk::EventsLog::BroadcastUnhandledException(
							__FUNCTION__,
							__FILE__,
							__LINE__);
						throw Exception("Failed to attach security");
					}
					boost::format securityStr("%1% from %2%");
					securityStr % security % security.GetSource().GetTag();
					securities.push_back(securityStr.str());
					return true;
				});
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
			int(Trait::Type) != int(MODULE_TYPE_SERVICE),
			"Wrong CreateStandaloneModuleInstance method choose.");
		CreateModuleInstance(tag, module);
	}

	////////////////////////////////////////////////////////////////////////////////

	std::list<std::string> ParseSupplierRequestList(
				const std::string &request)
			const {

		std::list<std::string> result;
		
		bool isSymbolListOpened = false;
		typedef boost::split_iterator<std::string::const_iterator> It;
		for (	It i = boost::make_split_iterator(
					request,
					boost::first_finder(",", boost::is_iequal()));
				i != It();
				++i) {
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
			if (isNewRequirement || result.empty()) {
				result.push_back(boost::copy_range<std::string>(*i));
				boost::trim(result.back());
				if (result.back().empty()) {
					result.pop_back();
				}
			} else {
				auto &line = result.back();
				line.push_back(',');
				line.insert(line.end(), boost::begin(*i), boost::end(*i));
				boost::trim(line);
				if (result.back().empty()) {
					result.pop_back();
				}
			}
		}

		if (isSymbolListOpened) {
			m_context.GetLog().Error(
				"Requirements syntax error:"
					" expected closing \"]\" for \"%1%\" in \"%2%\".",
					*result.rbegin(),
					request);
			throw Exception("Requirements syntax error");
		}

		return result;

	}

	SupplierRequest ParseSupplierRequest(const std::string &request) const {

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
			result.symbols.insert(Symbol());
			return result;
		}

		typedef boost::split_iterator<std::string::const_iterator> It;
		for (	It i = boost::make_split_iterator(
					symbolList,
					boost::first_finder(",", boost::is_iequal()));
				i != It();
				++i) {
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
					symbol,
					request);
				throw Exception("Requirements syntax error");
			}
			result.symbols.insert(symbol);
		}

		return result;

	}

	template<typename Module>
	void ReadRequirementsList(
				ModuleDll<Module> &module,
				const std::string &tag,
 				RequirementsList &result) {
		
		typedef ModuleTrait<Module> Trait;

		const auto &parseRequest = [&](
					const SupplierRequest &request,
					Module *uniqueInstance) {
			typedef ModuleTrait<Module> Trait;
			if (
					boost::iequals(
						request.tag,
						Constants::Services::level1Updates)) {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					uniqueInstance,
					SYSTEM_SERVICE_LEVEL1_UPDATES,
					request,
					result);
			} else if (
					boost::iequals(
						request.tag,
						Constants::Services::level1Ticks)) {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					uniqueInstance,
					SYSTEM_SERVICE_LEVEL1_TICKS,
					request,
					result);
			} else if (
					boost::iequals(
						request.tag,
						Constants::Services::bookUpdateTicks)) {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					uniqueInstance,
					SYSTEM_SERVICE_BOOK_UPDATE_TICKS,
					request,
					result);
			} else if (
					boost::iequals(
						request.tag,
						Constants::Services::trades)) {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					uniqueInstance,
					SYSTEM_SERVICE_TRADES,
					request,
					result);
			} else if (
					boost::iequals(
						request.tag,
						Constants::Services::brokerPositionsUpdates)) {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					uniqueInstance,
					SYSTEM_SERVICE_BROKER_POSITIONS_UPDATES,
					request,
					result);
			} else if (
					boost::iequals(
						request.tag,
						Constants::Services::bars)) {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					uniqueInstance,
					SYSTEM_SERVICE_BARS,
					request,
					result);
			} else {
				UpdateRequirementsList(
					Trait::GetType(),
					tag,
					uniqueInstance,
					request,
					result);
			}
		};

		{
			const auto &list = ParseSupplierRequestList(
				module.conf->ReadKey(Keys::requires, std::string()));
			foreach (const std::string &request, list) {
				parseRequest(ParseSupplierRequest(request), nullptr);
			}
		}

		const auto &parseInstanceRequest = [&](Module &instance) {
			std::string strList;
			try {
				strList = instance.GetRequiredSuppliers();
			} catch (...) {
				trdk::EventsLog::BroadcastUnhandledException(
					__FUNCTION__,
					__FILE__,
					__LINE__);
				// DLL will be unloaded, replacing exception:
				throw Exception("Failed to read module requirement list");
			}
			const auto &list = ParseSupplierRequestList(strList);
			foreach (const auto &request, list) {
				parseRequest(ParseSupplierRequest(request), &instance);
			}
		};
		ForEachModuleInstance(
			module,
			parseInstanceRequest,
			parseInstanceRequest);

	}

	void UpdateRequirementsList(
				ModuleType type,
				const std::string &tag,
				Module *uniqueInstance,
				SystemService requiredService,
				const SupplierRequest &supplierRequest,
				RequirementsList &list) {
		ApplyRequirementsListModifier(
			type,
			tag,
			uniqueInstance,
			[&](TagRequirementsList &requirements) {
				Assert(
					!requirements.uniqueInstance
					|| requirements.uniqueInstance == uniqueInstance);
				requirements.requiredSystemServices[requiredService].insert(
					supplierRequest.symbols.begin(),
					supplierRequest.symbols.end());
			},
			list);
	}

	void UpdateRequirementsList(
				ModuleType type,
				const std::string &tag,
				Module *uniqueInstance,
				const SupplierRequest &supplierRequest,
				RequirementsList &list) {
		ApplyRequirementsListModifier(
			type,
			tag,
			uniqueInstance,
			[&](TagRequirementsList &requirements) {
				auto &module
					= requirements.requiredModules[supplierRequest.tag];
#				ifdef DEV_VER
					if (supplierRequest.symbols.size() > 1) {
 						foreach (const auto &symbol, supplierRequest.symbols) {
 							Assert(symbol);
 						}
					}
#				endif
				module.insert(supplierRequest.symbols);
			},
			list);
	}

	void BindWithModuleRequirements(const RequirementsList &requirements) {
		foreach (
				const TagRequirementsList &moduleRequirements,
				requirements.get<BySubscriber>()) {
			static_assert(
				numberOfModuleTypes == 3,
				"Changed module type list.");
			switch (moduleRequirements.subscriberType) {
				case MODULE_TYPE_STRATEGY:
					BindModuleWithModuleRequirements(
						moduleRequirements,
						m_strategies);
					break;
				case MODULE_TYPE_SERVICE:
					BindModuleWithModuleRequirements(
						moduleRequirements,
						m_services);
					break;
				case MODULE_TYPE_OBSERVER:
					BindModuleWithModuleRequirements(
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
	
	void BindWithSystemRequirements(const RequirementsList &requirements) {
		foreach (
				const TagRequirementsList &moduleRequirements,
				requirements.get<BySubscriber>()) {
			static_assert(
				numberOfModuleTypes == 3,
				"Changed module type list.");
			switch (moduleRequirements.subscriberType) {
				case MODULE_TYPE_STRATEGY:
					BindModuleWithSystemRequirements(
						moduleRequirements,
						m_strategies);
					break;
				case MODULE_TYPE_SERVICE:
					BindModuleWithSystemRequirements(
						moduleRequirements,
						m_services);
					break;
				case MODULE_TYPE_OBSERVER:
					BindModuleWithSystemRequirements(
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
	void BindModuleWithModuleRequirements(
				const TagRequirementsList &requirements,
				std::map<std::string /*tag*/, ModuleDll<Module>> &modules) {

		typedef ModuleTrait<Module> Trait;
		AssertEq(int(Trait::Type), int(requirements.subscriberType));

		const auto &modulePos = modules.find(requirements.subscriberTag);
		Assert(modulePos != modules.end());
		if (modulePos == modules.end()) {
			return;
		}
		ModuleDll<Module> &module = modulePos->second;

		foreach (const auto &requirement, requirements.requiredModules) {
			const auto &requirementTag = requirement.first;
			const auto requredModulePos = m_services.find(requirementTag);
			if (requredModulePos == m_services.end()) {
				m_context.GetLog().Error(
					"Unknown service with tag \"%1%\" in requirement list.",
					requirementTag);
				throw Exception("Unknown service in requirement list");
			}
 			ModuleDll<Service> &requredModule = requredModulePos->second;
 			foreach (const std::set<Symbol> &symbols, requirement.second) {

				const auto createBySymbolSet = [&](
							const std::set<Symbol> &symbols,
							std::string &symbolsStrList)
						-> boost::shared_ptr<Service> {
					std::list<std::string> symbolsStr;
					foreach (const Symbol &symbol, symbols) {
						Assert(symbol);
						symbolsStr.push_back(symbol.GetAsString());
					}
					std::string symbolsStrListTmp
						= boost::join(symbolsStr, ", ");
 					const auto requredServicePos
 						= requredModule.symbolInstances.find(symbols);
					boost::shared_ptr<Service> result;
 					if (	requredServicePos
							!= requredModule.symbolInstances.end()) {
						result = requredServicePos->second.GetObjPtr();
					} else {
						result = CreateModuleInstance(
							requirementTag,
							symbols,
							requredModule);
						Assert(
							requredModule.symbolInstances.find(symbols)
							!= requredModule.symbolInstances.end());
 					}
					symbolsStrListTmp.swap(symbolsStrList);
					return result;
				};

				if (symbols.size() == 1 && !*symbols.begin()) {

					foreach (auto &instance, module.symbolInstances) {
						std::string symbolsStrList;
						boost::shared_ptr<Service> requredService
							= createBySymbolSet(instance.first, symbolsStrList);
						requredService->RegisterSubscriber(*instance.second);
						instance.second->GetLog().Info(
							"Subscribed to \"%1%\" with security(ies) \"%2%\".",
							*requredService,
							symbolsStrList);
					}

				} else {

					std::string symbolsStrList;
					boost::shared_ptr<Service> requredService
						= createBySymbolSet(symbols, symbolsStrList);

					foreach (auto &instance, module.standaloneInstances) {
						requredService->RegisterSubscriber(*instance);
						instance->GetLog().Info(
							"Subscribed to \"%1%\" with security(ies) \"%2%\".",
							*requredService,
							symbolsStrList);
					}
					foreach (auto &instance, module.symbolInstances) {
						requredService->RegisterSubscriber(*instance.second);
						instance.second->GetLog().Info(
							"Subscribed to \"%1%\" with security(ies) \"%2%\".",
							*requredService,
							symbolsStrList);
					}

				}
				
 			}
		}

	}

	template<typename Module>
	void BindModuleWithSystemRequirements(
				const TagRequirementsList &requirements,
				std::map<std::string /*tag*/, ModuleDll<Module>> &modules) {

		typedef ModuleTrait<Module> Trait;
		AssertEq(int(Trait::Type), int(requirements.subscriberType));

		const auto &modulePos = modules.find(requirements.subscriberTag);
		Assert(modulePos != modules.end());
		if (modulePos == modules.end()) {
			return;
		}
		ModuleDll<Module> &module = modulePos->second;

		Module *uniqueInstance = nullptr;
		bool isUniqueInstanceStandalone = false;
		if (requirements.uniqueInstance) {
			uniqueInstance = boost::polymorphic_downcast<Module *>(
				requirements.uniqueInstance);
			isUniqueInstanceStandalone = false;
			foreach (auto &instance, module.standaloneInstances) {
				if (&*instance == uniqueInstance) {
					isUniqueInstanceStandalone = true;
					break;
				}
			}
#			ifdef DEV_VER
				if (!isUniqueInstanceStandalone) {
					bool isExist = false;
					foreach (auto &instance, module.symbolInstances) {
						if (&*instance.second == uniqueInstance) {
							isExist = true;
							break;
						}
					}
					Assert(isExist);
				}
#			endif
		}

		// Subscribing to system services:
		foreach (
				const auto &requirement,
				requirements.requiredSystemServices) {
			void (SubscriptionsManager::*subscribe)(Security &, Module &)
				= nullptr;
			static_assert(
				numberOfSystemServices == 6,
				"System service list changed.");
			switch (requirement.first) {
				case SYSTEM_SERVICE_LEVEL1_UPDATES:
					subscribe = &SubscriptionsManager::SubscribeToLevel1Updates;
					break;
				case SYSTEM_SERVICE_LEVEL1_TICKS:
					subscribe = &SubscriptionsManager::SubscribeToLevel1Ticks;
					break;
				case SYSTEM_SERVICE_BOOK_UPDATE_TICKS:
					subscribe
						= &SubscriptionsManager::SubscribeToBookUpdateTicks;
					break;
				case SYSTEM_SERVICE_TRADES:
					subscribe = &SubscriptionsManager::SubscribeToTrades;
					break;
				case SYSTEM_SERVICE_BROKER_POSITIONS_UPDATES:
					subscribe = &SubscriptionsManager
						::SubscribeToBrokerPositionUpdates;
					break;
				case SYSTEM_SERVICE_BARS:
					subscribe = &SubscriptionsManager::SubscribeToBars;
					break;
				default:
					AssertEq(SYSTEM_SERVICE_LEVEL1_UPDATES, requirement.first);
					break;
			}
			Assert(subscribe);
			if (!subscribe) {
				continue;
			}
			foreach (const Symbol &symbol, requirement.second) {
				if (symbol) {
					m_context.ForEachMarketDataSource(
						[&](MarketDataSource &source) -> bool {
							Security &security = source.GetSecurity(symbol);
							if (!uniqueInstance) {
								ForEachModuleInstance(
									module,
									[&](Module &instance) {
										SubscribeModuleStandaloneInstance(
											instance,
											subscribe,
											&security);
									},
									[&](Module &instance) {
										SubscribeModuleSymbolInstance(
											instance,
											subscribe,
											&security);
									});
							} else if (isUniqueInstanceStandalone) {
								SubscribeModuleStandaloneInstance(
									*uniqueInstance,
									subscribe,
									&security);
							} else {
								SubscribeModuleSymbolInstance(
									*uniqueInstance,
									subscribe,
									&security);
							}
							return true;
						});
				} else {
					if (!uniqueInstance) {
						ForEachModuleInstance(
							module,
							[&](Module &instance) {
								SubscribeModuleStandaloneInstance(
									instance,
									subscribe);
							},
							[&](Module &instance) {
								SubscribeModuleSymbolInstance(
									instance,
									subscribe);
							});
					} else if (isUniqueInstanceStandalone) {
						SubscribeModuleStandaloneInstance(
							*uniqueInstance,
							subscribe);
					} else {
						SubscribeModuleSymbolInstance(
							*uniqueInstance,
							subscribe);
					}
				}
			}
		}

	}

	template<typename Module, typename Pred>
	void ForEachSubscribedSecurity(
				Module &module,
				const Pred &pred) {
		const auto begin = module.GetSecurities().GetBegin();
		const auto end = module.GetSecurities().GetEnd();
		for (auto i = begin; i != end; ++i) {
			pred(*i);
		}
	}
	template<typename Callback>
	void ForEachSubscribedSecurity(
				Service &module,
				const Callback &callback) {
		const auto begin = module.GetSecurities().GetBegin();
		const auto end = module.GetSecurities().GetEnd();
		for (auto i = begin; i != end; ++i) {
			m_context.ForEachMarketDataSource(
				[&](MarketDataSource &source) -> bool {
					if (source != i->GetSource()) {
						return true;
					}
					callback(source.GetSecurity(i->GetSymbol()));
					return true;
				});
			
		}
	}

	template<typename Module>
	void SubscribeModuleStandaloneInstance(
				Module &instance,
				void (SubscriptionsManager::*subscribe)(Security &, Module &),
				Security *security = nullptr) {
		if (security) {
			(m_subscriptionsManager.*subscribe)(*security, instance);
			instance.RegisterSource(*security);
		} else {
			ForEachSubscribedSecurity(
				instance,
				[&](Security &subscribedSecurity) {
					(m_subscriptionsManager.*subscribe)(
						subscribedSecurity,
						instance);
				});
		}
	}

	template<typename Module>
	void SubscribeModuleSymbolInstance(
				Module &instance,
				void (SubscriptionsManager::*subscribe)(Security &, Module &),
				Security *security = nullptr) {
		if (security) {
			(m_subscriptionsManager.*subscribe)(
				*security,
				instance);
			instance.RegisterSource(*security);
		} else {
			ForEachSubscribedSecurity(
				instance,
				[&](Security &subscribedSecurity) {
					(m_subscriptionsManager.*subscribe)(
						subscribedSecurity,
						instance);
				});
		}
	}

	////////////////////////////////////////////////////////////////////////////////

	template<typename Module>
	std::set<Symbol> GetSymbolInstances(
				const ModuleTrait<Module> &,
				const std::string &tag,
				const IniSectionRef &conf) {

		typedef ModuleTrait<Module> Trait;
		static_assert(
			int(Trait::Type) != int(MODULE_TYPE_OBSERVER),
			"Wrong GetSymbolInstances method choose.");

		std::set<Symbol> result;

		if (!conf.IsKeyExist(Keys::instances)) {
			return result;
		}

		fs::path symbolsFilePath;
		if (dynamic_cast<const IniFile *>(&conf.GetBase())) {
			try {
 				symbolsFilePath = Normalize(
 					conf.ReadKey(Keys::instances),
 					boost::polymorphic_downcast<const IniFile *>(
							&conf.GetBase())
						->GetPath()
						.branch_path());
			} catch (const Lib::Ini::Error &ex) {
				m_context.GetLog().Error(
					"Failed to get symbol instances file: \"%1%\".",
					ex);
				throw;
			}
		} else {
			symbolsFilePath = conf.ReadFileSystemPath(Keys::instances);
		}
		m_context.GetLog().Debug(
			"Loading symbol instances from %1% for %2% \"%3%\"...",
			symbolsFilePath,
			Trait::GetName(false),
			tag);
		const IniFile symbolsIni(symbolsFilePath);
		std::set<Symbol> symbols = symbolsIni.ReadSymbols(
			m_context.GetSettings().GetDefaultExchange(),
			m_context.GetSettings().GetDefaultPrimaryExchange());

		try {
			foreach (const auto &iniSymbol, symbols) {
				result.insert(Symbol(iniSymbol));
			}
		} catch (const Lib::Ini::Error &ex) {
			m_context.GetLog().Error(
				"Failed to load securities for %2%: \"%1%\".",
				ex,
				tag);
			throw Lib::Ini::Error("Failed to load securities");
		}
	
		return result;

	}

	////////////////////////////////////////////////////////////////////////////////

	template<typename Module>
	ModuleDll<Module> LoadModuleDll(
				const IniSectionRef &conf,
				const std::string &tag)
			const {

		typedef ModuleTrait<Module> Trait;
		typedef typename Trait::Factory Factory;

		ModuleDll<Module> result = {};
		result.conf.reset(new IniSectionRef(conf));

		if (tag.empty()) {
			m_context.GetLog().Error(
				"Failed to get tag for %1% section \"%2%\".",
				Trait::GetName(false),
				result.conf);
			throw Lib::Ini::Error("Failed to load module");
		}

		fs::path modulePath;
		try {
			if (	!result.conf->IsKeyExist(Keys::module)
					&& !Trait::GetDefaultModule().empty()) {
				modulePath = Normalize(Trait::GetDefaultModule());
			} else {
				modulePath = result.conf->ReadFileSystemPath(Keys::module);
			}
		} catch (const Lib::Ini::Error &ex) {
			m_context.GetLog().Error(
				"Failed to get %1% module: \"%2%\".",
				Trait::GetName(false),
				ex);
			throw Lib::Ini::Error("Failed to load module");
		}

		result.dll.reset(
			new Dll(modulePath, conf.ReadBoolKey(Keys::Dbg::autoName, true)));
		//! @todo Workaround for g++ 4.8.2 for Ubuntu:
		Dll &dll = *result.dll;

		const bool isFactoreNameKeyExist
			= result.conf->IsKeyExist(Keys::factory);
		const std::string factoryName = isFactoreNameKeyExist
			?	result.conf->ReadKey(Keys::factory)
			:	Trait::GetDefaultFactory(tag);
		try {
			result.factory = dll.GetFunction<Factory>(factoryName);
		} catch (const Dll::DllFuncException &) {
			const std::string unifiedName
				= DefaultValues::Factories::factoryNameStart
					+ Trait::GetName(true);
			if (!isFactoreNameKeyExist) {
				result.factory = dll.GetFunction<Factory>(unifiedName);
			} else {
				if (
						boost::istarts_with(
							factoryName,
							DefaultValues::Factories::factoryNameStart)) {
					throw;
				}
				std::string appendedName
					= DefaultValues::Factories::factoryNameStart
					+ factoryName;
				if (!boost::iends_with(factoryName, Trait::GetName(true))) {
					appendedName += Trait::GetName(true);
				}
				try {
					result.factory
						= dll.GetFunction<Factory>(appendedName);
				} catch (const Dll::DllFuncException &) {
					try {
						result.factory
							= dll.GetFunction<Factory>(unifiedName);
					} catch (const Dll::DllFuncException &) {
						//...//
					}
				}
			}
			if (!result.factory) {
				throw;
			}
		}

		return result;

	}

	////////////////////////////////////////////////////////////////////////////////

private:

	Engine::Context &m_context;

	SubscriptionsManager &m_subscriptionsManager;

	Strategies *m_strategiesResult;
	Observers *m_observersResult;
	Services *m_servicesResult;
	ModuleList &m_moduleListResult;

	StrategyModules m_strategies;
	ObserverModules m_observers;
	ServiceModules m_services;

	const Lib::Ini &m_conf;

};

template<>
void ContextStateBootstrapper::CreateStandaloneModuleInstance(
			const std::string &tag,
			ModuleDll<Service> &) {
	typedef ModuleTrait<Service> Trait;
	m_context.GetLog().Debug(
		"%1% \"%2%\" instantiation delayed.",
		Trait::GetName(true),
		tag);
}


template<>
std::set<Symbol> ContextStateBootstrapper::GetSymbolInstances(
			const ModuleTrait<Observer> &,
			const std::string &,
			const IniSectionRef &) {
	return std::set<Symbol>();
}

////////////////////////////////////////////////////////////////////////////////

void Engine::BootContext(
			const Lib::Ini &conf,
			Context &context,
			TradeSystems &tradeSystemsRef,
			MarketDataSources &marketDataSourcesRef) {
	ContextBootstrapper(
			conf,
			context,
			tradeSystemsRef,
			marketDataSourcesRef)
		.Boot();
}

void Engine::BootContextState(
			const Lib::Ini &conf,
			Context &context,
			SubscriptionsManager &subscriptionsManagerRef,
			Strategies &strategiesRef,
			Observers &observersRef,
			Services &servicesRef,
			ModuleList &moduleListRef) {
	ContextStateBootstrapper(
			conf,
			context,
			subscriptionsManagerRef,
			strategiesRef,
			observersRef,
			servicesRef,
			moduleListRef)
		.Boot();
}

void Engine::BootNewStrategiesForContextState(
			const trdk::Lib::Ini &newStrategiesConf,
			Context &context,
			SubscriptionsManager &subscriptionsManagerRef,
			Strategies &strategiesRef,
			Services &servicesRef,
			ModuleList &moduleListRef) {
	ContextStateBootstrapper(
			newStrategiesConf,
			context,
			subscriptionsManagerRef,
			strategiesRef,
			servicesRef,
			moduleListRef)
		.Boot();
}

////////////////////////////////////////////////////////////////////////////////
