/**************************************************************************
 *   Created: 2012/07/08 14:06:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "FakeTradeSystem.hpp"
#include "Ini.hpp"
#include "Util.hpp"
#include "Dispatcher.hpp"
#include "Core/Algo.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Core/MarketDataSource.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

using namespace Trader;

namespace {

	typedef std::map<std::string, boost::shared_ptr<Security>> Securities;

	typedef DllObjectPtr<Algo> ModuleAlgo;
	typedef std::map<std::string /*tag*/, std::list<ModuleAlgo>> Algos;

}

namespace {

	std::string CreateSecuritiesKey(const IniFile::Symbol &symbol) {
		return Util::CreateSymbolFullStr(symbol.symbol, symbol.primaryExchange, symbol.exchange);
	}

	void LoadSecurities(
				const std::list<IniFile::Symbol> &symbols,
				boost::shared_ptr<TradeSystem> tradeSystem,
				Securities &securities,
				boost::shared_ptr<Settings> settings,
				const IniFile &ini) {
		const std::list<std::string> logMdSymbols
			= ini.ReadList(Ini::Sections::MarketData::Log::symbols, false);
		Securities securititesTmp(securities);
		foreach (const IniFile::Symbol &symbol, symbols) {
			const std::string key = CreateSecuritiesKey(symbol);
			if (securities.find(key) != securities.end()) {
				continue;
			}
			securititesTmp[key] = boost::shared_ptr<Security>(
				new Security(
					tradeSystem,
					symbol.symbol,
					symbol.primaryExchange,
					symbol.exchange,
					settings,
					find(logMdSymbols.begin(), logMdSymbols.end(), symbol.symbol) != logMdSymbols.end()));
			Log::Info("Loaded security \"%1%\".", securititesTmp[key]->GetFullSymbol());
		}
		securititesTmp.swap(securities);
	}

	void InitAlgo(
				const IniFile &ini,
				const std::string &section,
				boost::shared_ptr<TradeSystem> tradeSystem,
				Securities &securities,
				Algos &algos,
				boost::shared_ptr<Settings> settings)  {

		fs::path module;
		try {
			module = ini.ReadKey(section, Ini::Key::module, false);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get algo module: \"%1%\".", ex.what());
			throw IniFile::Error("Failed to load algo");
		}

		std::string fabricName;
		try {
			fabricName = ini.ReadKey(section, Ini::Key::fabric, false);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get algo fabric name module: \"%1%\".", ex.what());
			throw IniFile::Error("Failed to load algo");
		}

		const std::string tag = section.substr(Ini::Sections::algo.size());
		if (tag.empty()) {
			Log::Error("Failed to get tag for algo section \"%1%\".", section);
			throw IniFile::Error("Failed to load algo");
		}

		Log::Info("Loading objects for \"%1%\"...", tag);

		std::string symbolsFilePath;
		try {
			symbolsFilePath = ini.ReadKey(section, Ini::Key::symbols, false);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get symbols file: \"%1%\".", ex.what());
			throw;
		}

		Log::Info("Loading symbols from %1% for %2%...", symbolsFilePath, tag);
		const IniFile symbolsIni(symbolsFilePath, ini.GetPath().branch_path());
		const std::list<IniFile::Symbol> symbols = symbolsIni.ReadSymbols("SMART", "NASDAQ");
		try {
			LoadSecurities(symbols, tradeSystem, securities, settings, ini);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to load securities for %2%: \"%1%\".", ex.what(), tag);
			throw IniFile::Error("Failed to load algo");
		}

		boost::shared_ptr<Dll> dll;
		foreach (auto &as, algos) {
			foreach (auto &a, as.second) {
				if (a.GetDll()->GetFile() == module) {
					dll = a;
					break;
				}
			}
		}
		if (!dll) {
			dll.reset(new Dll(module, true));
		}
		const auto fabric
			= dll->GetFunction<
					boost::shared_ptr<Algo>(
						const std::string &tag,
						boost::shared_ptr<Security> security,
						const IniFile &ini,
						const std::string &section)>
				(fabricName);

		foreach (const auto &symbol, symbols) {
			Assert(securities.find(CreateSecuritiesKey(symbol)) != securities.end());
			boost::shared_ptr<Algo> newAlgo;
			try {
				newAlgo = fabric(
					tag,
 					securities[CreateSecuritiesKey(symbol)],
 					ini,
 					section);
			} catch (...) {
				Log::RegisterUnhandledException(__FUNCTION__, __FILE__, __LINE__, false);
				throw Exception("Failed to load algo");
			}
			algos[tag].push_back(DllObjectPtr<Algo>(dll, newAlgo));
			Log::Info(
				"Loaded algo \"%1%\" for \"%2%\" with tag \"%3%\".",
				newAlgo->GetName(),
				const_cast<const Algo &>(*newAlgo).GetSecurity()->GetFullSymbol(),
				tag);
		}

	}

	bool InitTrading(
				const IniFile &ini,
				boost::shared_ptr<TradeSystem> tradeSystem,
				Dispatcher &dispatcher,
				boost::shared_ptr<LiveMarketDataSource> marketDataSource,
				Algos &algos,
				boost::shared_ptr<Settings> settings)  {

		const std::set<std::string> sections = ini.ReadSectionsList();

		Securities securities;
		foreach (const auto &section, sections) {
			if (boost::starts_with(section, Ini::Sections::algo)) {
				Log::Info("Found algo section \"%1%\"...", section);
				try {
					InitAlgo(ini, section, tradeSystem, securities, algos, settings);
				} catch (const Exception &ex) {
					Log::Error(
						"Failed to load algo module from section \"%1%\": \"%2%\".",
						section,
						ex.what());
				}
			}
		}

		if (algos.empty()) {
			Log::Error("No algos loaded.");
		}

		foreach (auto &as, algos) {
			foreach (auto &a, as.second) {
				dispatcher.Register(a);
			}
		}

		Log::Info("Loaded %1% securities.", securities.size());
		Log::Info("Loaded %1% algos.", algos.size());

		try {
			Connect(*tradeSystem, ini, Ini::Sections::tradeSystem);
			Connect(*marketDataSource, ini, Ini::Sections::MarketData::Source::live);
		} catch (const Exception &ex) {
			Log::Error("Failed to make trading connections: \"%1%\".", ex.what());
			throw Exception("Failed to make trading connections");
		}

		return true;

	}

	void UpdateSettingsRuntime(
				const fs::path &iniFilePath,
				Algos &algos,
				Settings &settings) {
		Log::Info("Detected INI-file %1% modification, updating current settings...", iniFilePath);
		const IniFile ini(iniFilePath);
		std::set<std::string> sections;
		try {
			sections = ini.ReadSectionsList();
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get sections list: \"%1%\".", ex.what());
			return;
		}
		foreach (const auto &section, sections) {
			if (section == Ini::Sections::common) {
				try {
					settings.Update(ini, section);
				} catch (const Exception &ex) {
					Log::Error("Failed to update common settings: \"%1%\".", ex.what());
				}
				continue;
			}
			bool isError = false;
			std::string algoName;
			if (!boost::starts_with(section, Ini::Sections::algo)) {
				continue;
			}
			const std::string tag = section.substr(Ini::Sections::algo.size());
			if (tag.empty()) {
				Log::Error("Failed to get tag for algo section \"%1%\".", section);
				continue;
			}
			const Algos::iterator pos = algos.find(tag);
			if (pos == algos.end()) {
				Log::Warn(
					"Could not update current settings: could not find algo with tag \"%1%\".",
					tag);
				continue;
			}
			foreach (auto &a, pos->second) {
				AssertEq(a->GetTag(), tag);
				try {
					a->UpdateSettings(ini, section);
				} catch (const Exception &ex) {
					Log::Error("Failed to update current settings: \"%1%\".", ex.what());
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

	DllObjectPtr<LiveMarketDataSource> LoadLiveMarketDataSource(
				const IniFile &ini,
				const std::string &section) {
		const std::string module = ini.ReadKey(section, Ini::Key::module, false);
		const std::string fabricName = ini.ReadKey(section, Ini::Key::fabric, false);
		boost::shared_ptr<Dll> dll(new Dll(module, true));
		return DllObjectPtr<LiveMarketDataSource>(
			dll,
			dll->GetFunction<boost::shared_ptr<LiveMarketDataSource>()>(fabricName)());
	}

}

void Trade(const fs::path &iniFilePath) {

	Log::Info("Using %1% INI-file...", iniFilePath);
	const IniFile ini(iniFilePath);

	boost::shared_ptr<Settings> settings
		= Ini::LoadSettings(ini, boost::get_system_time(), false);

	DllObjectPtr<TradeSystem> tradeSystem
		= LoadTradeSystem(ini, Ini::Sections::tradeSystem);
	DllObjectPtr<LiveMarketDataSource> marketDataSource
		= LoadLiveMarketDataSource(ini, Ini::Sections::MarketData::Source::live);

	Dispatcher dispatcher(settings);

	Algos algos;
	if (	!InitTrading(
				ini,
				tradeSystem,
				dispatcher,
				marketDataSource,
				algos,
				settings)) {
		return;
	}

	foreach (auto &as, algos) {
		foreach (auto &a, as.second) {
			a->SubscribeToMarketData(marketDataSource);
		}
	}

	FileSystemChangeNotificator iniChangeNotificator(
		iniFilePath,
		boost::bind(
			&UpdateSettingsRuntime,
			boost::cref(iniFilePath),
			boost::ref(algos),
			boost::ref(*settings)));

	iniChangeNotificator.Start();
	dispatcher.Start();
	(*marketDataSource).Start();

	getchar();
	iniChangeNotificator.Stop();
	dispatcher.Stop();

}
