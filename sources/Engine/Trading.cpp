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
	typedef std::map<
			std::string /* strategy tag */,
			std::list<StrategyService>>
		StrategyServices;

	typedef DllObjectPtr<Observer> ModuleObserver;
	typedef std::map<std::string /*tag*/, ModuleObserver> Observers;

	typedef DllObjectPtr<Service> ModuleService;
	typedef std::map<IniFile::Symbol, ModuleService> ServicesBySymbol;
	typedef std::map<std::string /*tag*/, ServicesBySymbol> Services;

}

namespace {
	
	void LoadSecurities(
				const std::set<IniFile::Symbol> &symbols,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				Securities &securities,
				boost::shared_ptr<Settings> settings,
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
			Log::Info(
				"Loaded security \"%1%\".",
				securititesTmp[symbol]->GetFullSymbol());
		}
		securititesTmp.swap(securities);
	}

	std::string LoadModule(
				const IniFile &ini,
				const std::string &section,
				const std::string &tag,
				const char *moduleType,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				std::set<IniFile::Symbol> &symbols,
				Securities &securities,
				boost::shared_ptr<Settings> settings,
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
				ex.what());
			throw IniFile::Error("Failed to load module");
		}

		std::string fabricName;
		try {
			fabricName = ini.ReadKey(section, Ini::Key::fabric, false);
		} catch (const IniFile::Error &ex) {
			Log::Error(
				"Failed to get %1% fabric name module: \"%2%\".",
				moduleType,
				ex.what());
			throw IniFile::Error("Failed to load module");
		}

		Log::Info("Loading %1% objects for \"%2%\"...", moduleType, tag);

		std::string symbolsFilePath;
		try {
			symbolsFilePath
				= ini.ReadKey(section, Ini::Key::symbols, false);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get symbols file: \"%1%\".", ex.what());
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
			Log::Error(
				"Failed to load securities for %2%: \"%1%\".",
				ex.what(),
				tag);
			throw IniFile::Error("Failed to load strategy");
		}
	
		dll.reset(new Dll(module, true));
		return fabricName;

	}

	template<typename Module>
	void InitModuleBySymbol(
				const IniFile &ini,
				const std::string &section,
				const char *moduleType,
				const std::string &tag,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				Securities &securities,
				std::map<
						std::string,
						std::map<IniFile::Symbol, DllObjectPtr<Module>>>
					&modules,
				boost::shared_ptr<Settings> settings) {

		std::set<IniFile::Symbol> symbols;
		boost::shared_ptr<Dll> dll;
		const auto fabricName = LoadModule(
			ini,
			section,
			tag,
			moduleType,
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
						const IniFileSectionRef &ini)>
				(fabricName);

		foreach (const auto &symbol, symbols) {
			Assert(securities.find(symbol) != securities.end());
			boost::shared_ptr<Module> symbolInstance;
			try {
				symbolInstance = fabric(
					tag,
 					securities[symbol],
					IniFileSectionRef(ini, section));
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
				"Loaded %1% \"%2%\" for \"%3%\" with tag \"%4%\".",
				moduleType,
				symbolInstance->GetName(),
				const_cast<const Module &>(*symbolInstance)
					.GetSecurity()
					->GetFullSymbol(),
				symbolInstance->GetTag());
		}
	
	}

	void ReadServicesRequest(
				const IniFileSectionRef &ini,
				const std::string &tag,
				StrategyServices &strategyServices) {
		
		const std::string strList = ini.ReadKey(Ini::Key::services, true);
		if (strList.empty()) {
			return;
		}
		
		std::list<std::string> list;
		boost::split(list, strList, boost::is_any_of(","));
		const boost::regex expr("([^\\[]+)\\[(.+)\\]");
		foreach (std::string &serviceRequest, list) {
			boost::smatch what;
			const auto cleanRequest = boost::trim_copy(serviceRequest);
			if (!boost::regex_match(cleanRequest, what, expr)) {
				Log::Error(
					"Failed to parse service request \"%1%\" for \"%2%\".",
					serviceRequest,
					tag);
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
			strategyServices[tag].push_back(service);
		}

	}

	void InitStrategy(
				const IniFile &ini,
				const std::string &section,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				Securities &securities,
				Strategies &strategies,
				StrategyServices &strategyServices,
				boost::shared_ptr<Settings> settings) {
		const std::string tag = section.substr(Ini::Sections::strategy.size());
		InitModuleBySymbol(
			ini,
			section,
			"strategy",
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
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				Securities &securities,
				Observers &observers,
				boost::shared_ptr<Settings> settings)  {

		const std::string tag
			= section.substr(Ini::Sections::observer.size());

		std::set<IniFile::Symbol> symbols;
		boost::shared_ptr<Dll> dll;
		const auto fabricName = LoadModule(
			ini,
			section,
			tag,
			"observer",
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
		Log::Info(
			"Loaded observer \"%1%\" with tag \"%2%\".",
			newObserver->GetName(),
			newObserver->GetTag());
	
	}

	void InitService(
				const IniFile &ini,
				const std::string &section,
				boost::shared_ptr<TradeSystem> &tradeSystem,
				MarketDataSource &marketDataSource,
				Securities &securities,
				Services &services,
				boost::shared_ptr<Settings> settings)  {
		InitModuleBySymbol(
			ini,
			section,
			"service",
			section.substr(Ini::Sections::service.size()),
			tradeSystem,
			marketDataSource,
			securities,
			services,
			settings);
	}

	void BindStrategyWithService(
				const StrategyServices &binding,
				const Services &services,
				const IniFile &ini,
				const IniFile::Symbol &strategySymbol,
				Strategy &strategy) {
		const auto bindingInfo = binding.find(strategy.GetTag());
		if (bindingInfo == binding.end()) {
			return;
		}
		foreach (const auto &info, bindingInfo->second) {
			const auto service = services.find(info.tag);
			if (service == services.end()) {
				Log::Error(
					"Unknown service \"%1%\" provided for strategy \"%2%\".",
					info.tag,
					strategy.GetTag());
				throw Exception("Unknown service provided for strategy");
			}
			IniFile::Symbol bindedSymbol;
			Assert(!info.symbol.empty());
			if (info.symbol[0] == '$') {
				if (	!boost::equal(
							info.symbol.substr(1),
							Ini::Variables::currentSymbol)) {
					Log::Error(
						"Unknown service symbol \"%1%\" provided for strategy \"%2%\".",
						info.symbol,
						strategy.GetTag());
					throw Exception("Unknown service symbol provided for strategy");
				}
				bindedSymbol = strategySymbol;
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
					"Unknown service symbol \"%1%\" provided for strategy \"%2%\".",
					bindedSymbol,
					strategy.GetTag());
				throw Exception("Unknown service symbol provided for strategy");
			}
			strategy.NotifyServiceStart(serviceSymbol->second);
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

		const std::set<std::string> sections = ini.ReadSectionsList();

		Securities securities;
		StrategyServices strategyServices;
		foreach (const auto &section, sections) {
			if (boost::starts_with(section, Ini::Sections::strategy)) {
				Log::Info(
					"Found strategy section \"%1%\"...",
					section);
				try {
					InitStrategy(
						ini,
						section,
						tradeSystem,
						*marketDataSource,
						securities,
						strategies,
						strategyServices,
						settings);
				} catch (const Exception &ex) {
					Log::Error(
						"Failed to load strategy module from section \"%1%\": \"%2%\".",
						section,
						ex.what());
					throw Exception("Failed to load strategy module");
				}
			} else if (boost::starts_with(section, Ini::Sections::observer)) {
				Log::Info("Found observer section \"%1%\"...", section);
				try {
					InitObserver(
						ini,
						section,
						tradeSystem,
						*marketDataSource,
						securities,
						observers,
						settings);
				} catch (const Exception &ex) {
					Log::Error(
						"Failed to load observer module from section \"%1%\": \"%2%\".",
						section,
						ex.what());
					throw Exception("Failed to load observer module");
				}
			} else if (boost::starts_with(section, Ini::Sections::service)) {
				Log::Info("Found service section \"%1%\"...", section);
				try {
					InitService(
						ini,
						section,
						tradeSystem,
						*marketDataSource,
						securities,
						services,
						settings);
				} catch (const Exception &ex) {
					Log::Error(
						"Failed to load service module from section \"%1%\": \"%2%\".",
						section,
						ex.what());
					throw Exception("Failed to load service module");
				}
			}
		}

		if (strategies.empty()) {
			throw Exception("No strategies loaded");
		}

		foreach (auto &ss, strategies) {
			foreach (auto &strategy, ss.second) {
				BindStrategyWithService(
					strategyServices,
					services,
					ini,
					strategy.first,
					strategy.second);
				dispatcher.Register(strategy.second);
			}
		}
		foreach (auto &o, observers) {
			dispatcher.Register(o.second);
		}
		foreach (auto &ss, services) {
			foreach (auto &s, ss.second) {
				dispatcher.Register(s.second);
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
		std::set<std::string> sections;
		try {
			sections = ini.ReadSectionsList();
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get sections list: \"%1%\".", ex.what());
			return;
		}
		foreach (const auto &sectionName, sections) {
			if (sectionName == Ini::Sections::common) {
				try {
					settings.Update(ini, sectionName);
				} catch (const Exception &ex) {
					Log::Error(
						"Failed to update common settings: \"%1%\".",
						ex.what());
				}
				continue;
			}
			bool isError = false;
			std::string strategyName;
			if (!boost::starts_with(sectionName, Ini::Sections::strategy)) {
				continue;
			}
			const std::string tag
				= sectionName.substr(Ini::Sections::strategy.size());
			if (tag.empty()) {
				Log::Error(
					"Failed to get tag for strategy section \"%1%\".",
					sectionName);
				continue;
			}
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
						ex.what());
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
		Log::Error("Failed to init trading: \"%1%\".", ex.what());
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
		Log::Error("Failed to make trading connections: \"%1%\".", ex.what());
		throw Exception("Failed to make trading connections");
	}

	getchar();
	iniChangeNotificator.Stop();
	dispatcher.Stop();

}
