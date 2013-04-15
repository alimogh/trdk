/**************************************************************************
 *   Created: 2012/07/08 14:06:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Context.hpp"
#include "SubscriptionsManager.hpp"
#include "Ini.hpp"
#include "Util.hpp"
#include "Core/Strategy.hpp"
#include "Core/Observer.hpp"
#include "Core/Service.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Core/MarketDataSource.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;
namespace mi = boost::multi_index;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;

namespace {

	//////////////////////////////////////////////////////////////////////////

	typedef DllObjectPtr<Strategy> ModuleStrategy;
	typedef std::map<
			std::string /*tag*/,
			std::map<IniFile::Symbol, ModuleStrategy>>
		Strategies;

	//////////////////////////////////////////////////////////////////////////
	
	enum SystemService {
		SYSTEM_SERVICE_LEVEL1,
		SYSTEM_SERVICE_TRADES,
		numberOfSystemServices
	};

	//////////////////////////////////////////////////////////////////////////

	enum ModuleType {
		MODULE_TYPE_STRATEGY,
		MODULE_TYPE_SERVICE,
		MODULE_TYPE_OBSERVER,
		numberOfModuleTypes
	};

	template<typename Module>
	struct ModuleTrait {
		//...//
	};
	template<>
	struct ModuleTrait<Strategy> {
		static ModuleType GetType() {
			return MODULE_TYPE_STRATEGY;
		}
		static const char * GetName() {
			return "strategy";
		}
		static const std::string & GetDefaultFabric() {
			return Ini::DefaultValues::Fabrics::strategy;
		}
		static std::string GetDefaultModule() {
			return std::string();
		}
	};
	template<>
	struct ModuleTrait<Service> {
		static ModuleType GetType() {
			return MODULE_TYPE_SERVICE;
		}
		static const char * GetName() {
			return "service";
		}
		static const std::string & GetDefaultFabric() {
			return Ini::DefaultValues::Fabrics::service;
		}
		static const std::string & GetDefaultModule() {
			return Ini::DefaultValues::Modules::service;
		}
	};
	template<>
	struct ModuleTrait<Observer> {
		static ModuleType GetType() {
			return MODULE_TYPE_OBSERVER;
		}
		static const char * GetName() {
			return "observer";
		}
		static const std::string & GetDefaultFabric() {
			return Ini::DefaultValues::Fabrics::observer;
		}
		static std::string GetDefaultModule() {
			return std::string();
		}
	};

	//////////////////////////////////////////////////////////////////////////

	struct Use {
		
		ModuleType subscriberType;
		std::string subscriberTag;
	
		struct Module {
			
			std::string tag;
			std::string symbol;

			bool operator <(const Module &rhs) const {
				return tag < rhs.tag || (tag == rhs.tag && symbol < rhs.symbol);
			}

		};
		std::set<Module> modules;
		
		std::bitset<numberOfSystemServices> systemServices;
	
	};

	struct BySubscriber {
		//...//
	};

	typedef boost::multi_index_container<
			Use,
			mi::indexed_by<
				mi::ordered_unique<
					mi::tag<BySubscriber>,
						mi::composite_key<
							Use,
							mi::member<
								Use,
								ModuleType,
								&Use::subscriberType>,
							mi::member<
								Use,
								std::string,
								&Use::subscriberTag>>>>>
		Uses;
	typedef Uses::index<BySubscriber>::type SubscribedBySubcriber;

	namespace Detail {
		void Update(
					ModuleType type,
					const std::string &tag,
					const boost::function<void (Use &)> &modifier,
					Uses &list) {
			auto &index = list.get<BySubscriber>();
			auto pos = index.find(boost::make_tuple(type, tag));
			if (pos != index.end()) {
				Verify(index.modify(pos, modifier));
			} else {
				Use use = {};
				use.subscriberType = type;
				use.subscriberTag = tag;
				modifier(use);
				list.insert(use);
			}
		}
	}
	

	void Update(
				ModuleType type,
				const std::string &tag,
				SystemService service,
				Uses &list) {
		Detail::Update(
			type,
			tag,
			[&service](Use &use) {
				use.systemServices.set(service);
			},
			list);
	}

	void Update(
				ModuleType type,
				const std::string &tag,
				const std::string &moduleTag,
				const std::string &moduleSymbol,
				Uses &list) {
		Use::Module module = {
				moduleTag,
				moduleSymbol
			};
		Detail::Update(
			type,
			tag,
			[&module](Use &use) {
				use.modules.insert(module);
			},
			list);
	}

	//////////////////////////////////////////////////////////////////////////
	
	typedef DllObjectPtr<Observer> ModuleObserver;
	typedef std::map<std::string /*tag*/, ModuleObserver> Observers;

	typedef DllObjectPtr<Service> ModuleService;
	typedef std::map<IniFile::Symbol, ModuleService> ServicesBySymbol;
	typedef std::map<std::string /*tag*/, ServicesBySymbol> Services;

	//////////////////////////////////////////////////////////////////////////

	bool GetModuleSection(
				const std::string &sectionName,
				std::string &typeResult,
				std::string &tagResult) {
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
				std::string &tagResult) {
		return GetModuleSection(sectionName, typeResult, tagResult)
			?	new IniFileSectionRef(ini, sectionName)
			:	nullptr;
	}

	//////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////

class Engine::Context::Implementation : private boost::noncopyable {

public:

	class State;

public:

	Engine::Context &m_context;

	const IniFile m_configurationFile;
	Settings m_settings;

	DllObjectPtr<TradeSystem> m_tradeSystem;
	DllObjectPtr<MarketDataSource> m_marketDataSource;

	std::unique_ptr<State> m_state;

 public:

	explicit Implementation(
				Engine::Context &context,
				const fs::path &configurationFilePath,
				bool isReplayMode)
			: m_context(context),
			m_configurationFile(configurationFilePath),
			m_settings(
				IniFileSectionRef(m_configurationFile, Ini::Sections::common),
				boost::get_system_time(),
				isReplayMode,
				m_context.GetLog()),
			m_tradeSystem(LoadTradeSystem()),
			m_marketDataSource(LoadMarketDataSource()) {
		//...//
	}

private:

	DllObjectPtr<TradeSystem> LoadTradeSystem() {
		const IniFileSectionRef configurationSection(
			m_configurationFile,
			Ini::Sections::tradeSystem);
		const std::string module
			= configurationSection.ReadKey(Ini::Keys::module, false);
		const std::string fabricName
			= configurationSection.ReadKey(Ini::Keys::fabric, false);
		boost::shared_ptr<Dll> dll(new Dll(module, true));
		typedef boost::shared_ptr<TradeSystem> (Proto)(
			const IniFileSectionRef &,
			Log &);
		try {
			return DllObjectPtr<TradeSystem>(
				dll,
				dll->GetFunction<Proto>(fabricName)(
					configurationSection,
					m_context.GetLog()));
		} catch (...) {
			trdk::Log::RegisterUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__,
				false);
			throw Exception("Failed to load trade system module");
		}
	}

	DllObjectPtr<MarketDataSource> LoadMarketDataSource() {
		const IniFileSectionRef configurationSection(
			m_configurationFile,
			Ini::Sections::MarketData::source);
		const std::string module
			= configurationSection.ReadKey(Ini::Keys::module, false);
		const std::string fabricName
			= configurationSection.ReadKey(Ini::Keys::fabric, false);
		boost::shared_ptr<Dll> dll(new Dll(module, true));
		typedef boost::shared_ptr<MarketDataSource> (Proto)(
			const IniFileSectionRef &,
			Log &);
		try {
			return DllObjectPtr<MarketDataSource>(
				dll,
				dll->GetFunction<Proto>(fabricName)(
					configurationSection,
					m_context.GetLog()));
		} catch (...) {
			trdk::Log::RegisterUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__,
				false);
			throw Exception("Failed to load market data source module");
		}
	}

};

//////////////////////////////////////////////////////////////////////////

class Engine::Context::Implementation::State : private boost::noncopyable {

private:

	typedef std::map<IniFile::Symbol, boost::shared_ptr<Security>> Securities;

public:

	explicit State(
				Context &context,
				const IniFile &configurationFile)
			: m_context(context),
			m_subscriptionsManager(m_context) {
		try {
			Init(configurationFile);
		} catch (const Exception &ex) {
			GetLog().Error("Failed to init engine context: \"%1%\".", ex);
			throw Exception("Failed to init engine context");
		}
		m_subscriptionsManager.Activate();
	}

protected:

	Context::Log & GetLog() const {
		return m_context.GetLog();
	}

private:

	void Init(const IniFile &configurationFile) {

		const auto sections = configurationFile.ReadSectionsList();

		Uses uses;
		foreach (const auto &sectionName, sections) {
			std::string type;
			std::string tag;
			std::unique_ptr<const IniFileSectionRef> section(
				GetModuleSection(configurationFile, sectionName, type, tag));
			if (!section) {
				continue;
			}
			if (boost::iequals(type, Ini::Sections::strategy)) {
				try {
					InitStrategy(*section, tag, uses);
				} catch (const Exception &ex) {
					GetLog().Error(
						"Failed to load strategy module from section \"%1%\":"
							" \"%2%\".",
						boost::make_tuple(
							boost::cref(*section),
							boost::cref(ex)));
					throw Exception("Failed to load strategy module");
				}
			} else if (boost::iequals(type, Ini::Sections::observer)) {
				try {
					InitObserver(*section, tag);
				} catch (const Exception &ex) {
					GetLog().Error(
						"Failed to load observer module from section \"%1%\":"
							" \"%2%\".",
					boost::make_tuple(
						boost::cref(*section),
						boost::cref(ex)));
					throw Exception("Failed to load observer module");
				}
			} else if (boost::iequals(type, Ini::Sections::service)) {
				try {
					InitService(*section, tag, uses);
				} catch (const Exception &ex) {
					GetLog().Error(
						"Failed to load service module from section \"%1%\":"
							" \"%2%\".",
					boost::make_tuple(
						boost::cref(*section),
						boost::cref(ex)));
					throw Exception("Failed to load service module");
				}
			} else {
				AssertFail("Unknown module type");
			}
		}

		if (m_strategies.empty()) {
			throw Exception("No strategies loaded");
		}

		foreach (auto &ss, m_strategies) {
			foreach (auto &strategy, ss.second) {
				BindWithServices(
					uses,
					strategy.first,
					*strategy.second,
					configurationFile);
			}
		}
		foreach (auto &o, m_observers) {
			m_subscriptionsManager.SubscribeToTrades(o.second);
		}
		foreach (auto &ss, m_services) {
			foreach (auto &service, ss.second) {
				BindWithServices(
					uses,
					service.first,
					*service.second,
					configurationFile);
			}
		}

		GetLog().Info("Loaded %1% securities.", m_securities.size());
		GetLog().Info("Loaded %1% strategies.", m_strategies.size());
		GetLog().Info("Loaded %1% observers.", m_observers.size());
		GetLog().Info("Loaded %1% services.", m_services.size());

	}

	void InitStrategy(
				const IniFileSectionRef &section,
				const std::string &tag,
				Uses &uses) {
		GetLog().Debug("Found strategy section \"%1%\"...", section);
		InitModuleBySymbol(section, tag, m_strategies);
		ReadUses(section, tag, ModuleTrait<Strategy>::GetType(), uses);
	}

	void InitObserver(
				const IniFileSectionRef &section,
				const std::string &tag)  {

		GetLog().Debug("Found observer section \"%1%\"...", section);

		std::set<IniFile::Symbol> symbols;
		boost::shared_ptr<Dll> dll;
		const auto fabricName = LoadModule<Observer>(
			section,
			tag,
			symbols,
			dll);

		const auto fabric
			= dll->GetFunction<
					boost::shared_ptr<trdk::Observer>(
						trdk::Context &,
						const std::string &tag,
						const Observer::NotifyList &,
						const IniFileSectionRef &)>
				(fabricName);

		Observer::NotifyList notifyList;
		foreach (const auto &symbol, symbols) {
			Assert(m_securities.find(symbol) != m_securities.end());
			notifyList.push_back(m_securities[symbol]);
		}

		boost::shared_ptr<Observer> newObserver;
		try {
			newObserver = fabric(m_context, tag, notifyList, section);
		} catch (...) {
			trdk::Log::RegisterUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__,
				false);
			throw Exception("Failed to load observer");
		}

		Assert(m_observers.find(tag) == m_observers.end());
		m_observers[tag] = DllObjectPtr<Observer>(dll, newObserver);
		GetLog().Info("Loaded \"%1%\".", *newObserver);
	
	}

	void InitService(
				const IniFileSectionRef &section,
				const std::string &tag,
				Uses &uses) {
		GetLog().Debug("Found service section \"%1%\"...", section);
		if (	boost::iequals(tag, Ini::Constants::Services::level1)
				|| boost::iequals(tag, Ini::Constants::Services::trades)) {
			GetLog().Error(
				"System predefined service name used for section %1%: \"%2%\".",
				boost::make_tuple(boost::cref(section), boost::cref(tag)));
			throw Exception("System predefined service name used");
		}
		InitModuleBySymbol(section, tag, m_services);
		ReadUses(section, tag, ModuleTrait<Service>::GetType(), uses);
	}

	template<typename Module>
	void InitModuleBySymbol(
				const IniFileSectionRef &section,
				const std::string &tag,
				std::map<
						std::string,
						std::map<IniFile::Symbol, DllObjectPtr<Module>>>
					&modules) {

		std::set<IniFile::Symbol> symbols;
		boost::shared_ptr<Dll> dll;
		const auto fabricName = LoadModule<Module>(
			section,
			tag,
			symbols,
			dll);

		const auto fabric
			= dll->GetFunction<
					boost::shared_ptr<Module>(
						trdk::Context &,
						const std::string &tag,
						Security &security,
						const IniFileSectionRef &configuration)>
				(fabricName);

		foreach (const auto &symbol, symbols) {
			const auto securityIt = m_securities.find(symbol);
			Assert(securityIt != m_securities.end());
			boost::shared_ptr<Module> symbolInstance;
			try {
				symbolInstance = fabric(
					m_context,
					tag,
					*securityIt->second,
					section);
			} catch (...) {
				trdk::Log::RegisterUnhandledException(
					__FUNCTION__,
					__FILE__,
					__LINE__,
					false);
				throw Exception("Failed to create module instance");
			}
			Assert(
				modules[symbolInstance->GetTag()].find(symbol)
					== modules[symbolInstance->GetTag()].end());
			modules[symbolInstance->GetTag()][symbol]
				= DllObjectPtr<Module>(dll, symbolInstance);
			GetLog().Info(
				"Loaded \"%1%\" for \"%2%\".",
				boost::make_tuple(
					boost::cref(*symbolInstance),
					boost::cref(symbolInstance->GetSecurity())));
		}
	
	}

	void ReadUses(
				const IniFileSectionRef &ini,
				const std::string &tag,
				ModuleType moduleType,
 				Uses &uses)
			const {
		
		const std::string strList = ini.ReadKey(Ini::Keys::uses, true);
		if (strList.empty()) {
			return;
		}
		
		std::list<std::string> list;
		boost::split(list, strList, boost::is_any_of(","));
		foreach (std::string &request, list) {
			boost::trim(request);
			if (	boost::iequals(
						request,
						Ini::Constants::Services::level1)) {
				Update(moduleType, tag, SYSTEM_SERVICE_LEVEL1, uses);
			} else if (
					boost::iequals(
						request,
						Ini::Constants::Services::trades)) {
				Update(moduleType, tag, SYSTEM_SERVICE_TRADES, uses);
			} else {
				Update(moduleType, tag, request, std::string(), uses);
			}
		}

	}

	template<typename Module>
	void BindWithServices(
				const Uses &binding,
				const IniFile::Symbol &symbol,
				Module &module,
				const IniFile &configurationFile) {
		
		const auto &bindingIndex = binding.get<BySubscriber>();
		const auto bindingInfo = bindingIndex.find(
			boost::make_tuple(ModuleTrait<Module>::GetType(), module.GetTag()));
		if (bindingInfo == bindingIndex.end()) {
			GetLog().Error("No data sources specified for \"%1%\".", module);
			throw Exception("No data sources specified");
		}

		bool isAnyService = false;

		foreach (const auto &info, bindingInfo->modules) {
			const auto service = m_services.find(info.tag);
			if (service == m_services.end()) {
				GetLog().Error(
					"Unknown service \"%1%\" provided for \"%2%\".",
					boost::make_tuple(
						boost::cref(info.tag),
						boost::cref(module)));
				throw Exception("Unknown service provided");
			}
			const IniFile::Symbol bindedSymbol = info.symbol.empty()
				?	symbol
				:	IniFile::ParseSymbol(
						info.symbol,
						configurationFile.ReadKey(
							Ini::Sections::defaults,
							Ini::Keys::exchange,
							false),
						configurationFile.ReadKey(
							Ini::Sections::defaults,
							Ini::Keys::primaryExchange,
							false));
			const auto serviceSymbol = service->second.find(bindedSymbol);
			if (serviceSymbol == service->second.end()) {
				GetLog().Error(
					"Unknown service symbol \"%1%\" provided for \"%2%\".",
					boost::make_tuple(
						boost::cref(bindedSymbol),
						boost::cref(module)));
				throw Exception("Unknown service symbol provided");
			}
			serviceSymbol->second->RegisterSubscriber(module);
			module.OnServiceStart(serviceSymbol->second);
			isAnyService = true;
		}

		if (bindingInfo->systemServices[SYSTEM_SERVICE_LEVEL1]) {
			m_subscriptionsManager.SubscribeToLevel1(module);
			isAnyService = true;
		}

		if (bindingInfo->systemServices[SYSTEM_SERVICE_TRADES]) {
			m_subscriptionsManager.SubscribeToTrades(module);
			isAnyService = true;
		}

		if (!isAnyService) {
			GetLog().Error("No services provided for \"%1%\".", module);
			throw Exception("No services provided");
		}

	}

private:

	void LoadSecurities(
				const IniFile &configurationFile,
				const std::set<IniFile::Symbol> &symbols) {
		const std::list<std::string> logMdSymbols = configurationFile.ReadList(
			Ini::Sections::MarketData::Log::symbols,
			false);
		foreach (const IniFile::Symbol &symbol, symbols) {
			if (m_securities.find(symbol) != m_securities.end()) {
				continue;
			}
			m_securities[symbol]
				= m_context.GetMarketDataSource().CreateSecurity(
					m_context,
					symbol.symbol,
					symbol.primaryExchange,
					symbol.exchange,
					find(
							logMdSymbols.begin(),
							logMdSymbols.end(),
							symbol.symbol)
						!= logMdSymbols.end());
			GetLog().Info("Loaded security \"%1%\".", *m_securities[symbol]);
		}
	}

	template<typename Module>
	std::string LoadModule(
				const IniFileSectionRef &configurationSection,
				const std::string &tag,
				std::set<IniFile::Symbol> &symbols,
				boost::shared_ptr<Dll> &dll) {

		typedef ModuleTrait<Module> Trait;

		if (tag.empty()) {
			GetLog().Error(
				"Failed to get tag for %1% section \"%2%\".",
				boost::make_tuple(
					boost::cref(Trait::GetName()),
					boost::cref(configurationSection)));
			throw IniFile::Error("Failed to load module");
		}

		fs::path module;
		try {
			if (	!configurationSection.IsKeyExist(Ini::Keys::module)
					&& !Trait::GetDefaultModule().empty()) {
				module = Normalize(Trait::GetDefaultModule());
			} else {
				module = configurationSection.ReadFileSystemPath(Ini::Keys::module, false);
			}
		} catch (const IniFile::Error &ex) {
			GetLog().Error(
				"Failed to get %1% module: \"%2%\".",
				boost::make_tuple(
					boost::cref(Trait::GetName()),
					boost::cref(ex)));
			throw IniFile::Error("Failed to load module");
		}

		std::string fabricName;
		if (configurationSection.IsKeyExist(Ini::Keys::fabric)) {
			try {
				fabricName = configurationSection.ReadKey(Ini::Keys::fabric, false);
			} catch (const IniFile::Error &ex) {
				GetLog().Error(
					"Failed to get %1% fabric name module: \"%2%\".",
					boost::make_tuple(
						boost::cref(Trait::GetName()),
						boost::cref(ex)));
				throw IniFile::Error("Failed to load module");
			}
		} else {
			fabricName = Trait::GetDefaultFabric();
		}

		GetLog().Debug(
			"Loading %1% objects for \"%2%\"...",
			boost::make_tuple(
				boost::cref(Trait::GetName()),
				boost::cref(tag)));

		fs::path symbolsFilePath;
		try {
			symbolsFilePath = configurationSection.ReadFileSystemPath(
				Ini::Keys::symbols,
				false);
		} catch (const IniFile::Error &ex) {
			GetLog().Error("Failed to get symbols file: \"%1%\".", ex);
			throw;
		}

		GetLog().Info(
			"Loading symbols from %1% for %2% \"%3%\"...",
			boost::make_tuple(
				boost::cref(symbolsFilePath),
				boost::cref(Trait::GetName()),
				boost::cref(tag)));
		const IniFile symbolsIni(symbolsFilePath);
		symbols = symbolsIni.ReadSymbols(
			configurationSection.GetBase().ReadKey(
				Ini::Sections::defaults,
				Ini::Keys::exchange,
				false),
			configurationSection.GetBase().ReadKey(
				Ini::Sections::defaults,
				Ini::Keys::primaryExchange,
				false));
		try {
			LoadSecurities(configurationSection.GetBase(), symbols);
		} catch (const IniFile::Error &ex) {
			GetLog().Error(
				"Failed to load securities for %2%: \"%1%\".",
				boost::make_tuple(
					boost::cref(ex),
					boost::cref(tag)));
			throw IniFile::Error("Failed to load strategy");
		}
	
		dll.reset(new Dll(module, true));
		return fabricName;

	}


private:

	Context &m_context;

	SubscriptionsManager m_subscriptionsManager;

	Securities m_securities;
	Strategies m_strategies;
	Observers m_observers;
	Services m_services;

};

//////////////////////////////////////////////////////////////////////////

Engine::Context::Context(
			const fs::path &configurationFilePath,
			bool isReplayMode) {
	m_pimpl = new Implementation(*this, configurationFilePath, isReplayMode);
}

Engine::Context::~Context() {
	if (m_pimpl->m_state) {
		Stop();
	}
	delete m_pimpl;
}

void Engine::Context::Start() {
	GetLog().Info(
		"Starting with configuration file %1%...",
		m_pimpl->m_configurationFile.GetPath());
	Assert(!m_pimpl->m_state);
	if (m_pimpl->m_state) {
		GetLog().Warn("Already started!");
		return;
	}
	std::unique_ptr<Implementation::State> state(
		new Implementation::State(*this, m_pimpl->m_configurationFile));
	try {
		Connect(
			GetTradeSystem(),
			m_pimpl->m_configurationFile,
			Ini::Sections::tradeSystem);
		Connect(GetMarketDataSource());
	} catch (const Exception &ex) {
		GetLog().Error("Failed to make trading connections: \"%1%\".", ex);
		throw Exception("Failed to make trading connections");
	}
	m_pimpl->m_state.reset(state.release());
}

void Engine::Context::Stop() {
	GetLog().Info("Stopping...");
	m_pimpl->m_state.reset();
}

MarketDataSource & Engine::Context::GetMarketDataSource() {
	return m_pimpl->m_marketDataSource;
}

const MarketDataSource & Engine::Context::GetMarketDataSource() const {
	return const_cast<Context *>(this)->GetMarketDataSource();
}

TradeSystem & Engine::Context::GetTradeSystem() {
	return m_pimpl->m_tradeSystem;
}

const TradeSystem & Engine::Context::GetTradeSystem() const {
	return const_cast<Context *>(this)->GetTradeSystem();
}

const Settings & Engine::Context::GetSettings() const {
	return m_pimpl->m_settings;
}

//////////////////////////////////////////////////////////////////////////
