/**************************************************************************
 *   Created: 2012/07/08 14:06:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Ini.hpp"
#include "Util.hpp"
#include "Dispatcher.hpp"
#include "Core/Strategy.hpp"
#include "Core/Observer.hpp"
#include "Core/Service.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Core/MarketDataSource.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace Trader;
using namespace Trader::Lib;
using namespace Trader::Engine;

namespace {

	typedef std::map<
			IniFile::Symbol,
			boost::shared_ptr<Security>>
		Securities;

	typedef DllObjectPtr<Strategy> ModuleStrategy;
	typedef std::map<
			std::string /*tag*/,
			std::map<IniFile::Symbol, ModuleStrategy>>
		Strategies;

	struct StrategyService {
		std::string tag;
		std::string symbol;
	};
	
	enum SystemService {
		SYSTEM_SERVICE_LEVEL1,
		SYSTEM_SERVICE_TRADES,
		numberOfSystemServices
	};

	typedef std::map<
			std::string /* strategy tag */,
			std::tuple<
				std::list<StrategyService>,
				std::bitset<numberOfSystemServices>>>
		StrategyServices;
	
	typedef DllObjectPtr<Observer> ModuleObserver;
	typedef std::map<std::string /*tag*/, ModuleObserver> Observers;

	typedef DllObjectPtr<Service> ModuleService;
	typedef std::map<IniFile::Symbol, ModuleService> ServicesBySymbol;
	typedef std::map<std::string /*tag*/, ServicesBySymbol> Services;

	template<typename T>
	struct ModuleNamePolicy {
		//...//
	};
	template<>
	struct ModuleNamePolicy<Strategy> {
		static const char * GetName() {
			return "strategy";
		}
	};
	template<>
	struct ModuleNamePolicy<Observer> {
		static const char * GetName() {
			return "observer";
		}
	};
	template<>
	struct ModuleNamePolicy<Service> {
		static const char * GetName() {
			return "service";
		}
	};

	bool GetModuleSection(
				const std::string &section,
				std::string &typeResult,
				std::string &tagResult) {
		std::list<std::string> subs;
		boost::split(subs, section, boost::is_any_of("."));
		if (subs.empty()) {
			return false;
		} else if (
				!boost::iequals(*subs.begin(), Ini::Sections::strategy)
				&& !boost::iequals(*subs.begin(), Ini::Sections::observer)
				&& !boost::iequals(*subs.begin(), Ini::Sections::service)) {
			return false;
		} else if (subs.size() != 2 || subs.rbegin()->empty()) {
			char *detail = subs.size() < 2 || subs.rbegin()->empty()
				?	"Expected tag name after dot"
				:	"No extra dots allowed";
			Log::Error(
				"Wrong module section name format: \"%1%\". %2%.",
				section,
				detail);
			throw Exception("Wrong module section name format");
		}
		subs.begin()->swap(typeResult);
		subs.rbegin()->swap(tagResult);
		return true;
	}

}

namespace {
	
	void LoadSecurities(
				const std::set<IniFile::Symbol> &symbols,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				Securities &securities,
				boost::shared_ptr<const Settings> settings,
				const IniFile &ini) {
		const std::list<std::string> logMdSymbols
			= ini.ReadList(Ini::Sections::MarketData::Log::symbols, false);
		Securities securititesTmp(securities);
		foreach (const IniFile::Symbol &symbol, symbols) {
			if (securities.find(symbol) != securities.end()) {
				continue;
			}
			securititesTmp[symbol] = marketDataSource.CreateSecurity(
				tradeSystem,
				symbol.symbol,
				symbol.primaryExchange,
				symbol.exchange,
				settings,
				find(logMdSymbols.begin(), logMdSymbols.end(), symbol.symbol)
					!= logMdSymbols.end());
			Log::Info("Loaded security \"%1%\".", *securititesTmp[symbol]);
		}
		securititesTmp.swap(securities);
	}

	std::string LoadModule(
				const IniFile &ini,
				const std::string &section,
				const std::string &tag,
				const char *const moduleType,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				std::set<IniFile::Symbol> &symbols,
				Securities &securities,
				boost::shared_ptr<const Settings> settings,
				boost::shared_ptr<Dll> &dll) {

		if (tag.empty()) {
			Log::Error(
				"Failed to get tag for %1% section \"%2%\".",
				moduleType,
				section);
			throw IniFile::Error("Failed to load module");
		}

		fs::path module;
		try {
			module = ini.ReadKey(section, Ini::Key::module, false);
		} catch (const IniFile::Error &ex) {
			Log::Error(
				"Failed to get %1% module: \"%2%\".",
				moduleType,
				ex);
			throw IniFile::Error("Failed to load module");
		}

		std::string fabricName;
		try {
			fabricName = ini.ReadKey(section, Ini::Key::fabric, false);
		} catch (const IniFile::Error &ex) {
			Log::Error(
				"Failed to get %1% fabric name module: \"%2%\".",
				moduleType,
				ex);
			throw IniFile::Error("Failed to load module");
		}

		Log::Info("Loading %1% objects for \"%2%\"...", moduleType, tag);

		std::string symbolsFilePath;
		try {
			symbolsFilePath
				= ini.ReadKey(section, Ini::Key::symbols, false);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get symbols file: \"%1%\".", ex);
			throw;
		}

		Log::Info(
			"Loading symbols from \"%1%\" for %2% \"%3%\"...",
			symbolsFilePath,
			moduleType,
			tag);
		const IniFile symbolsIni(
			symbolsFilePath,
			ini.GetPath().branch_path());
		symbols = symbolsIni.ReadSymbols(
			ini.ReadKey(
				Ini::Sections::defaults,
				Ini::Key::exchange,
				false),
			ini.ReadKey(
				Ini::Sections::defaults,
				Ini::Key::primaryExchange,
				false));
		try {
			LoadSecurities(
				symbols,
				tradeSystem,
				marketDataSource,
				securities,
				settings,
				ini);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to load securities for %2%: \"%1%\".", ex, tag);
			throw IniFile::Error("Failed to load strategy");
		}
	
		dll.reset(new Dll(module, true));
		return fabricName;

	}

	template<typename Module>
	void InitModuleBySymbol(
				const IniFile &ini,
				const std::string &section,
				const std::string &tag,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				Securities &securities,
				std::map<
						std::string,
						std::map<IniFile::Symbol, DllObjectPtr<Module>>>
					&modules,
				boost::shared_ptr<const Settings> settings) {

		std::set<IniFile::Symbol> symbols;
		boost::shared_ptr<Dll> dll;
		const auto fabricName = LoadModule(
			ini,
			section,
			tag,
			ModuleNamePolicy<Module>::GetName(),
			tradeSystem,
			marketDataSource,
			symbols,
			securities,
			settings,
			dll);

		const auto fabric
			= dll->GetFunction<
					boost::shared_ptr<Module>(
						const std::string &tag,
						boost::shared_ptr<Security> security,
						const IniFileSectionRef &ini,
						boost::shared_ptr<const Settings>)>
				(fabricName);

		foreach (const auto &symbol, symbols) {
			Assert(securities.find(symbol) != securities.end());
			boost::shared_ptr<Module> symbolInstance;
			try {
				symbolInstance = fabric(
					tag,
 					securities[symbol],
					IniFileSectionRef(ini, section),
					settings);
			} catch (...) {
				Log::RegisterUnhandledException(
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
			Log::Info(
				"Loaded \"%1%\" for \"%2%\".",
				*symbolInstance,
				*symbolInstance->GetSecurity());
		}
	
	}

	void ReadServicesRequest(
				const IniFileSectionRef &ini,
				const std::string &tag,
				StrategyServices &services) {
		
		const std::string strList = ini.ReadKey(Ini::Key::services, true);
		if (strList.empty()) {
			return;
		}
		
		std::list<std::string> list;
		boost::split(list, strList, boost::is_any_of(","));
		const boost::regex expr(
			"([^\\[]+)\\[(.+)\\]",
			boost::regex_constants::icase);
		foreach (std::string &serviceRequest, list) {
			if (	boost::iequals(
						serviceRequest,
						Ini::Constants::Services::level1)) {
				std::get<1>(services[tag]).set(SYSTEM_SERVICE_LEVEL1);
			} else if (
					boost::iequals(
						serviceRequest,
						Ini::Constants::Services::trades)) {
				std::get<1>(services[tag]).set(SYSTEM_SERVICE_TRADES);
			} else {
				boost::smatch what;
				const auto cleanRequest = boost::trim_copy(serviceRequest);
				if (!boost::regex_match(cleanRequest, what, expr)) {
					Log::Error(
						"Failed to parse service request \"%1%\" for \"%2%\"."
							" Expected: \"%1%[$%3%]\".",
						serviceRequest,
						tag,
						Ini::Variables::currentSymbol);
					throw Exception("Failed to load strategy");
				}
				if (what.str(2)[0] == '$') {
					if (	!boost::equal(
								what.str(2).substr(1),
								Ini::Variables::currentSymbol)) {
						Log::Error(
							"Failed to parse service request \"%1%\""
								" (unknown variable) for \"%2%\".",
							serviceRequest,
							tag);
						throw Exception("Failed to load strategy");
					}
				}
				StrategyService service = {};
				service.tag = what.str(1);
				service.symbol = what.str(2);
				std::get<0>(services[tag]).push_back(service);
			}
		}

	}

	void InitStrategy(
				const IniFile &ini,
				const std::string &section,
				const std::string &tag,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				Securities &securities,
				Strategies &strategies,
				StrategyServices &strategyServices,
				boost::shared_ptr<Settings> settings) {
		Log::Info("Found strategy section \"%1%\"...", section);
		InitModuleBySymbol(
			ini,
			section,
			tag,
			tradeSystem,
			marketDataSource,
			securities,
			strategies,
			settings);
		ReadServicesRequest(
			IniFileSectionRef(ini, section),
			tag,
			strategyServices);
	}

	void InitObserver(
				const IniFile &ini,
				const std::string &section,
				const std::string &tag,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				Securities &securities,
				Observers &observers,
				boost::shared_ptr<Settings> settings)  {

		Log::Info("Found observer section \"%1%\"...", section);

		std::set<IniFile::Symbol> symbols;
		boost::shared_ptr<Dll> dll;
		const auto fabricName = LoadModule(
			ini,
			section,
			tag,
			ModuleNamePolicy<Observer>::GetName(),
			tradeSystem,
			marketDataSource,
			symbols,
			securities,
			settings,
			dll);

		const auto fabric
			= dll->GetFunction<
					boost::shared_ptr<Trader::Observer>(
						const std::string &tag,
						const Observer::NotifyList &,
						boost::shared_ptr<Trader::TradeSystem>,
						const IniFile &,
						const std::string &section)>
				(fabricName);

		Observer::NotifyList notifyList;
		foreach (const auto &symbol, symbols) {
			Assert(securities.find(symbol) != securities.end());
			notifyList.push_back(securities[symbol]);
		}

		boost::shared_ptr<Observer> newObserver;
		try {
			newObserver = fabric(tag, notifyList, tradeSystem, ini, section);
		} catch (...) {
			Log::RegisterUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__,
				false);
			throw Exception("Failed to load observer");
		}

		Assert(observers.find(tag) == observers.end());
		observers[tag] = DllObjectPtr<Observer>(dll, newObserver);
		Log::Info("Loaded \"%1%\".", *newObserver);
	
	}

	void InitService(
				const IniFile &ini,
				const std::string &section,
				const std::string &tag,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				Securities &securities,
				Services &services,
				StrategyServices &strategyServices,
				boost::shared_ptr<Settings> settings)  {
		Log::Info("Found service section \"%1%\"...", section);
		if (	boost::iequals(tag, Ini::Constants::Services::level1)
				|| boost::iequals(tag, Ini::Constants::Services::trades)) {
			Log::Info(
				"System predefined service name used for section %1%: \"%2%\".",
				section,
				tag);
			throw Exception("System predefined service name used");
		}
		InitModuleBySymbol(
			ini,
			section,
			tag,
			tradeSystem,
			marketDataSource,
			securities,
			services,
			settings);
		ReadServicesRequest(
			IniFileSectionRef(ini, section),
			tag,
			strategyServices);
	}

	template<typename Module>
	void BindWithServices(
				const StrategyServices &binding,
				Services &services,
				const IniFile &ini,
				const IniFile::Symbol &symbol,
				Module &module,
				Dispatcher & dispatcher) {
		
		const auto bindingInfo = binding.find(module.GetTag());
		if (bindingInfo == binding.end()) {
			Log::Error("No services provided for \"%1%\".", module);
			throw Exception("No services provided");
		}

		bool isAnyService = false;

		foreach (const auto &info, std::get<0>(bindingInfo->second)) {
			const auto service = services.find(info.tag);
			if (service == services.end()) {
				Log::Error(
					"Unknown service \"%1%\" provided for \"%2%\".",
					info.tag,
					module);
				throw Exception("Unknown service provided");
			}
			IniFile::Symbol bindedSymbol;
			Assert(!info.symbol.empty());
			if (info.symbol[0] == '$') {
				if (	!boost::equal(
							info.symbol.substr(1),
							Ini::Variables::currentSymbol)) {
					Log::Error(
						"Unknown service symbol \"%1%\" provided for \"%2%\".",
						info.symbol,
						module);
					throw Exception("Unknown service symbol provided");
				}
				bindedSymbol = symbol;
			} else {
				bindedSymbol = IniFile::ParseSymbol(
					info.symbol,
					ini.ReadKey(
						Ini::Sections::defaults,
						Ini::Key::exchange,
						false),
					ini.ReadKey(
						Ini::Sections::defaults,
						Ini::Key::primaryExchange,
						false));
			}
			const auto serviceSymbol = service->second.find(bindedSymbol);
			if (serviceSymbol == service->second.end()) {
				Log::Error(
					"Unknown service symbol \"%1%\" provided for \"%2%\".",
					bindedSymbol,
					module);
				throw Exception("Unknown service symbol provided");
			}
			serviceSymbol->second->RegisterSubscriber(module);
			module.NotifyServiceStart(serviceSymbol->second);
			isAnyService = true;
		}

		if (std::get<1>(bindingInfo->second)[SYSTEM_SERVICE_LEVEL1]) {
			dispatcher.SubscribeToLevel1(module);
			isAnyService = true;
		}

		if (std::get<1>(bindingInfo->second)[SYSTEM_SERVICE_TRADES]) {
			dispatcher.SubscribeToNewTrades(module);
			isAnyService = true;
		}

		if (!isAnyService) {
			Log::Error("No services provided for \"%1%\".", module);
			throw Exception("No services provided");
		}

	}

	void InitTrading(
				const IniFile &ini,
				boost::shared_ptr<TradeSystem> tradeSystem,
				Dispatcher &dispatcher,
				boost::shared_ptr<MarketDataSource> marketDataSource,
				Strategies &strategies,
				Observers &observers,
				Services &services,
				boost::shared_ptr<Settings> settings)  {

		const auto sections = ini.ReadSectionsList();

		Securities securities;
		StrategyServices strategyServices;
		foreach (const auto &section, sections) {
			std::string type;
			std::string tag;
			if (!GetModuleSection(section, type, tag)) {
				continue;
			}
			if (boost::iequals(type, Ini::Sections::strategy)) {
				try {
					InitStrategy(
						ini,
						section,
						tag,
						tradeSystem,
						*marketDataSource,
						securities,
						strategies,
						strategyServices,
						settings);
				} catch (const Exception &ex) {
					Log::Error(
						"Failed to load strategy module from section \"%1%\":"
							" \"%2%\".",
						section,
						ex);
					throw Exception("Failed to load strategy module");
				}
			} else if (boost::iequals(type, Ini::Sections::observer)) {
				try {
					InitObserver(
						ini,
						section,
						tag,
						tradeSystem,
						*marketDataSource,
						securities,
						observers,
						settings);
				} catch (const Exception &ex) {
					Log::Error(
						"Failed to load observer module from section \"%1%\":"
							" \"%2%\".",
						section,
						ex);
					throw Exception("Failed to load observer module");
				}
			} else if (boost::iequals(type, Ini::Sections::service)) {
				try {
					InitService(
						ini,
						section,
						tag,
						tradeSystem,
						*marketDataSource,
						securities,
						services,
						strategyServices,
						settings);
				} catch (const Exception &ex) {
					Log::Error(
						"Failed to load service module from section \"%1%\": \"%2%\".",
						section,
						ex);
					throw Exception("Failed to load service module");
				}
			} else {
				AssertFail("Unknown module type");
			}
		}

		if (strategies.empty()) {
			throw Exception("No strategies loaded");
		}

		foreach (auto &ss, strategies) {
			foreach (auto &strategy, ss.second) {
				BindWithServices(
					strategyServices,
					services,
					ini,
					strategy.first,
					*strategy.second,
					dispatcher);
			}
		}
		foreach (auto &o, observers) {
			dispatcher.SubscribeToNewTrades(o.second);
		}
		foreach (auto &ss, services) {
			foreach (auto &service, ss.second) {
				BindWithServices(
					strategyServices,
					services,
					ini,
					service.first,
					*service.second,
					dispatcher);
			}
		}

		Log::Info("Loaded %1% securities.", securities.size());
		Log::Info("Loaded %1% strategies.", strategies.size());
		Log::Info("Loaded %1% observers.", observers.size());
		Log::Info("Loaded %1% services.", services.size());

	}

	void UpdateSettingsRuntime(
				const fs::path &iniFilePath,
				Strategies &strategies,
				Settings &settings) {
		Log::Info(
			"Detected INI-file %1% modification, updating current settings...",
			iniFilePath);
		const IniFile ini(iniFilePath);
		IniFile::SectionList sections;
		try {
			sections = ini.ReadSectionsList();
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get sections list: \"%1%\".", ex);
			return;
		}
		foreach (const auto &sectionName, sections) {
			if (boost::iequals(sectionName, Ini::Sections::common)) {
				try {
					settings.Update(ini, sectionName);
				} catch (const Exception &ex) {
					Log::Error(
						"Failed to update common settings: \"%1%\".",
						ex);
				}
				continue;
			}
			std::string type;
			std::string tag;
			if (	!GetModuleSection(sectionName, type, tag)
					|| !boost::iequals(tag, Ini::Sections::strategy)) {
				continue;
			}
			bool isError = false;
			const Strategies::iterator pos = strategies.find(tag);
			if (pos == strategies.end()) {
				Log::Warn(
					"Could not update current settings:"
						" could not find strategy with tag \"%1%\".",
					tag);
				continue;
			}
			foreach (auto &a, pos->second) {
				AssertEq(a.second->GetTag(), tag);
				try {
					a.second->UpdateSettings(
						IniFileSectionRef(ini, sectionName));
				} catch (const Exception &ex) {
					Log::Error(
						"Failed to update current settings: \"%1%\".",
						ex);
					isError = true;
					break;
				}
				if (isError) {
					break;
				}
			}
		}
		Log::Info("Current settings update competed.");
	}

	DllObjectPtr<TradeSystem> LoadTradeSystem(
				const IniFile &ini,
				const std::string &section) {
		const std::string module = ini.ReadKey(section, Ini::Key::module, false);
		const std::string fabricName = ini.ReadKey(section, Ini::Key::fabric, false);
		boost::shared_ptr<Dll> dll(new Dll(module, true));
		return DllObjectPtr<TradeSystem>(
			dll,
			dll->GetFunction<boost::shared_ptr<TradeSystem>()>(fabricName)());
	}

	DllObjectPtr<MarketDataSource> LoadMarketDataSource(
				const IniFile &ini,
				const std::string &section) {
		const std::string module = ini.ReadKey(
			section,
			Ini::Key::module,
			false);
		const std::string fabricName = ini.ReadKey(
			section,
			Ini::Key::fabric,
			false);
		boost::shared_ptr<Dll> dll(new Dll(module, true));
		try {
			typedef boost::shared_ptr<MarketDataSource> (Proto)(
				const IniFile &,
				const std::string &);
			return DllObjectPtr<MarketDataSource>(
				dll,
				dll->GetFunction<Proto>(fabricName)(
					ini,
					Ini::Sections::MarketData::source));
		} catch (...) {
			Log::RegisterUnhandledException(
				__FUNCTION__,
				__FILE__,
				__LINE__,
				false);
			throw Exception("Failed to load market data source");
		}
	}

}

void Trade(const fs::path &iniFilePath, bool isReplayMode) {

	Log::Info("Using %1% INI-file...", iniFilePath);
	const IniFile ini(iniFilePath);

	boost::shared_ptr<Settings> settings
		= Ini::LoadSettings(ini, boost::get_system_time(), isReplayMode);

	DllObjectPtr<TradeSystem> tradeSystem
		= LoadTradeSystem(ini, Ini::Sections::tradeSystem);
	DllObjectPtr<MarketDataSource> marketDataSource
		= LoadMarketDataSource(
			ini,
			Ini::Sections::MarketData::source);

	Dispatcher dispatcher(settings);

	Strategies strategies;
	Observers observers;
	Services services;
	try {
		InitTrading(
			ini,
			tradeSystem,
			dispatcher,
			marketDataSource,
			strategies,
			observers,
			services,
			settings);
	} catch (const Exception &ex) {
		Log::Error("Failed to init trading: \"%1%\".", ex);
		return;
	}

	FileSystemChangeNotificator iniChangeNotificator(
		iniFilePath,
		boost::bind(
			&UpdateSettingsRuntime,
			boost::cref(iniFilePath),
			boost::ref(strategies),
			boost::ref(*settings)));

	iniChangeNotificator.Start();
	dispatcher.Start();

	try {
		Connect(*tradeSystem, ini, Ini::Sections::tradeSystem);
		Connect(*marketDataSource);
	} catch (const Exception &ex) {
		Log::Error("Failed to make trading connections: \"%1%\".", ex);
		throw Exception("Failed to make trading connections");
	}

	getchar();
	iniChangeNotificator.Stop();
	dispatcher.Stop();

}
