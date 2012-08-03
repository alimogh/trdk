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
#include "Strategies/QuickArbitrage/QuickArbitrageAskBid.hpp"
#include "Strategies/Level2MarketArbitrage/Level2MarketArbitrage.hpp"
#include "IqFeed/IqFeedClient.hpp"
#include "InteractiveBrokers/InteractiveBrokersTradeSystem.hpp"
#include "Dispatcher.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"

namespace pt = boost::posix_time;
namespace fs = boost::filesystem;

namespace {

	typedef std::map<std::string, boost::shared_ptr<Security>> Securities;
	typedef std::list<boost::shared_ptr<Algo>> Algos;

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

		std::string algoName;
		try {
			if (!ini.IsKeyExist(section, Ini::Key::algo)) {
				return;
			}
			algoName = ini.ReadKey(section, Ini::Key::algo, false);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get strategy algo: \"%1%\".", ex.what());
			throw;
		}
		if (	algoName != Ini::Algo::QuickArbitrage::old
				&& algoName != Ini::Algo::QuickArbitrage::askBid
				&& algoName != Ini::Algo::level2MarketArbitrage) {
			return;
		}

		std::string tag;
		try {
			tag = ini.ReadKey(section, Ini::Key::tag, false);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get strategy object tag: \"%1%\".", ex.what());
			throw;
		}

		Log::Info("Loading strategy objects for \"%1%\"...", tag);
		
		std::string symbolsFilePath;
		try {
			symbolsFilePath = ini.ReadKey(section, Ini::Key::symbols, false);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to get symbols file: \"%1%\".", ex.what());
			throw;
		}

		Log::Info("Loading symbols from %1%...", symbolsFilePath);
		const IniFile symbolsIni(symbolsFilePath, ini.GetPath().branch_path());
		const std::list<IniFile::Symbol> symbols = symbolsIni.ReadSymbols("SMART", "NASDAQ");
		try {
			LoadSecurities(symbols, tradeSystem, securities, settings, ini);
		} catch (const IniFile::Error &ex) {
			Log::Error("Failed to load securities: \"%1%\".", ex.what());
			throw;
		}

		foreach (const auto &symbol, symbols) {
			
			Assert(securities.find(CreateSecuritiesKey(symbol)) != securities.end());
			
			try {
			
				boost::shared_ptr<Algo> algo;
				if (algoName == Ini::Algo::QuickArbitrage::old) {
					AssertFail("algoName == Ini::Algo::QuickArbitrage::old");
// 					algo.reset(
// 						new Strategies::QuickArbitrage::Old(
// 							tag,
// 							securities[CreateSecuritiesKey(symbol)],
// 							ini,
// 							section));
				} else if (algoName == Ini::Algo::QuickArbitrage::askBid) {
					algo.reset(
						new Strategies::QuickArbitrage::AskBid(
							tag,
							securities[CreateSecuritiesKey(symbol)],
							ini,
							section));
				} else if (algoName == Ini::Algo::level2MarketArbitrage) {
					algo.reset(
						new Strategies::Level2MarketArbitrage::Algo(
							tag,
							securities[CreateSecuritiesKey(symbol)],
							ini,
							section));
				} else {
					AssertFail("Unknown algo in INI file.");
				}
				algos.push_back(algo);
			
				Log::Info(
					"Loaded strategy \"%1%\" for \"%2%\".",
					algo->GetName(),
					const_cast<const Algo &>(*algo).GetSecurity()->GetFullSymbol());
		
			} catch (const Exception &ex) {
				Log::Error("Failed to load strategy: \"%1%\".", ex.what());
				throw;
			}
		
		}

	}

	void InitTrading(
				const fs::path &iniFilePath,
				boost::shared_ptr<TradeSystem> tradeSystem,
				Dispatcher &dispatcher,
				IqFeedClient &marketDataSource,
				Algos &algos,
				boost::shared_ptr<Settings> settings)  {

		Log::Info("Using %1% file for algo options...", iniFilePath);
		const IniFile ini(iniFilePath);
		const std::set<std::string> sections = ini.ReadSectionsList();

		Securities securities;
		foreach (const auto &section, sections) {
			if (boost::starts_with(section, Ini::Sections::strategy)) {
				Log::Info("Found section \"%1%\"...", section);
				InitAlgo(ini, section, tradeSystem, securities, algos, settings);
			}
		}

		foreach (auto a, algos) {
			dispatcher.Register(a);
		}

		Log::Info("Loaded %1% securities.", securities.size());
		Log::Info("Loaded %1% strategies.", algos.size());

		Connect(*tradeSystem, *settings);
		Connect(marketDataSource, *settings);

	}

	void UpdateSettingsRuntime(
				const fs::path &iniFilePath,
				Algos &algos,
				Settings &settings) {
		Log::Info("Detected INI-file %1% modification, updating current settings...", iniFilePath);
		const IniFile ini(iniFilePath);
		const std::set<std::string> sections = ini.ReadSectionsList();
		foreach (const auto &section, sections) {
			if (section == Ini::Sections::common) {
				settings.Update(ini, section);
				continue;
			} else if (!boost::starts_with(section, Ini::Sections::strategy)) {
				continue;
			}
			bool isError = false;
			std::string algoName;
			try {
				algoName = ini.ReadKey(section, Ini::Key::algo, false);
			} catch (const IniFile::Error &ex) {
				Log::Error("Failed to get strategy algo: \"%1%\".", ex.what());
				throw;
			}
			foreach (auto &a, algos) {
				if (algoName == Ini::Algo::level2MarketArbitrage) {
					if (!dynamic_cast<Strategies::Level2MarketArbitrage::Algo *>(a.get())) {
						continue;
					}
				} else if (algoName == Ini::Algo::QuickArbitrage::old) {
					AssertFail("algoName == Ini::Algo::QuickArbitrage::old");
// 					if (!dynamic_cast<Strategies::QuickArbitrage::Old *>(a.get())) {
// 						continue;
// 					}
				} else if (algoName == Ini::Algo::QuickArbitrage::askBid) {
					if (!dynamic_cast<Strategies::QuickArbitrage::AskBid *>(a.get())) {
						continue;
					}
				} else {
					continue;
				}
				try {
					a->UpdateSettings(ini, section);
				} catch (const std::exception &ex) {
					Log::Error("Failed to update current settings: \"%1%\".", ex.what());
					isError = true;
					break;
				}
			}
			if (isError) {
				break;
			}
		}
		Log::Info("Current settings update competed.");
	}

}

void Trade(const fs::path &iniFilePath) {

	boost::shared_ptr<Settings> settings
		= Ini::LoadSettings(iniFilePath, boost::get_system_time(), false);
	boost::shared_ptr<InteractiveBrokersTradeSystem> tradeSystem(
		new InteractiveBrokersTradeSystem);
	IqFeedClient marketDataSource;
	Dispatcher dispatcher(settings);

	Algos algos;
	InitTrading(iniFilePath, tradeSystem, dispatcher, marketDataSource, algos, settings);
	
	foreach (auto &a, algos) {
		a->SubscribeToMarketData(marketDataSource, *tradeSystem);
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
	getchar();
	iniChangeNotificator.Stop();
	dispatcher.Stop();
	algos.clear();

}

void ReplayTrading(const fs::path &iniFilePath, int argc, const char *argv[]) {

	Log::Info("!!! REPLAY MODE !!! REPLAY MODE !!! REPLAY MODE !!! REPLAY MODE !!!");

	Assert(argc >= 2 && std::string(argv[1]) == "replay");

	if (argc != 3) {
		throw Exception("Failed to get request parameters (replay date)");
	}

	boost::shared_ptr<Settings> settings = Ini::LoadSettings(
		iniFilePath,
		pt::time_from_string((boost::format("%1% 00:00:00") % argv[2]).str())
			- Util::GetEdtDiff(),
		true);
	Log::Info(
		"Replaying trade period: %1% - %2%.",
		settings->GetCurrentTradeSessionStartTime() + Util::GetEdtDiff(),
		settings->GetCurrentTradeSessionEndime() + Util::GetEdtDiff());

	boost::shared_ptr<TradeSystem> tradeSystem(new FakeTradeSystem);
	IqFeedClient marketDataSource;
	Dispatcher dispatcher(settings);
	Algos algos;
	InitTrading(iniFilePath, tradeSystem, dispatcher, marketDataSource, algos, settings);

	foreach (auto &a, algos) {
		a->RequestHistory(
			marketDataSource,
			settings->GetCurrentTradeSessionStartTime(),
			settings->GetCurrentTradeSessionEndime());
	}

	dispatcher.Start();
	Log::Info("!!! REPLAY MODE !!! REPLAY MODE !!! REPLAY MODE !!! REPLAY MODE !!!");
	getchar();
	dispatcher.Stop();
	algos.clear();

	Log::Info("!!! REPLAY MODE !!! REPLAY MODE !!! REPLAY MODE !!! REPLAY MODE !!!");

}
