/**************************************************************************
 *   Created: 2012/07/08 14:06:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Strategies/QuickArbitrage/QuickArbitrageOld.hpp"
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

	typedef std::map<std::string, boost::shared_ptr<DynamicSecurity>> Securities;
	typedef std::list<boost::shared_ptr<Algo>> Algos;

	namespace Ini {
		namespace Sections {
			const std::string common = "Common";
			namespace Algo {
				namespace QuickArbitrage {
					const std::string old = "Algo.QuickArbitrage.Old";
					const std::string askBid = "Algo.QuickArbitrage.AskBid";
				}
				const std::string level2MarketArbitrage = "Algo.Level2MarketArbitrage";
			}
			namespace MarketData {
				namespace Log {
					const std::string symbols = "MarketData.Log.Symbols";
				}
			}
		}
		namespace Key {
			const std::string symbols = "symbols";
		}
	}

}

namespace {

	std::string CreateSecuritiesKey(const IniFile::Symbol &symbol) {
		return Util::CreateSymbolFullStr(symbol.symbol, symbol.primaryExchange, symbol.exchange);
	}

	void Connect(TradeSystem &tradeSystem) {
		for ( ; ; ) {
			try {
				tradeSystem.Connect();
				break;
			} catch (const TradeSystem::ConnectError &) {
				boost::this_thread::sleep(pt::seconds(5));
			}
		}
	}

	void Connect(IqFeedClient &marketDataSource) {
		for ( ; ; ) {
			try {
				marketDataSource.Connect();
				break;
			} catch (const IqFeedClient::ConnectError &) {
				boost::this_thread::sleep(pt::seconds(5));
			}
		}
	}

	void LoadSecurities(
				const std::list<IniFile::Symbol> &symbols,
				boost::shared_ptr<TradeSystem> tradeSystem,
				Securities &securitites,
				boost::shared_ptr<Settings> settings,
				const IniFile &ini) {
		const std::list<std::string> logMdSymbols
			= ini.ReadList(Ini::Sections::MarketData::Log::symbols, false);
		Securities securititesTmp(securitites);
		foreach (const IniFile::Symbol &symbol, symbols) {
			const std::string key = CreateSecuritiesKey(symbol);
			if (securitites.find(key) != securitites.end()) {
				continue;
			}
			securititesTmp[key] = boost::shared_ptr<DynamicSecurity>(
				new DynamicSecurity(
					tradeSystem,
					symbol.symbol,
					symbol.primaryExchange,
					symbol.exchange,
					settings,
					find(logMdSymbols.begin(), logMdSymbols.end(), symbol.symbol) != logMdSymbols.end()));
			Log::Info("Loaded security \"%1%\".", securititesTmp[key]->GetFullSymbol());
		}
		securititesTmp.swap(securitites);
	}

	boost::shared_ptr<Settings> LoadOptions(const fs::path &iniFilePath) {
		Log::Info("Using %1% file for common options...", iniFilePath);
		return boost::shared_ptr<Settings>(new Settings(IniFile(iniFilePath), Ini::Sections::common));
	}

	void InitAlgo(
				const IniFile &ini,
				const std::string &section,
				boost::shared_ptr<TradeSystem> tradeSystem,
				Securities &securities,
				Algos &algos,
				boost::shared_ptr<Settings> settings)  {

		if (	section != Ini::Sections::Algo::QuickArbitrage::old
				&& section != Ini::Sections::Algo::QuickArbitrage::askBid
				&& section != Ini::Sections::Algo::level2MarketArbitrage) {
			return;
		}

		Log::Info("Loading strategy objects...");

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
				if (section == Ini::Sections::Algo::QuickArbitrage::old) {
					algo.reset(
						new Strategies::QuickArbitrage::Old(
							securities[CreateSecuritiesKey(symbol)],
							ini,
							section));
				} else if (section == Ini::Sections::Algo::QuickArbitrage::askBid) {
					algo.reset(
						new Strategies::QuickArbitrage::AskBid(
							securities[CreateSecuritiesKey(symbol)],
							ini,
							section));
				} else if (section == Ini::Sections::Algo::level2MarketArbitrage) {
					algo.reset(
						new Strategies::Level2MarketArbitrage::Algo(
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
		const std::list<std::string> sections = ini.ReadSectionsList();

		Securities securities;
		foreach (const auto &section, sections) {
			if (section != Ini::Sections::common) {
				Log::Info("Found section \"%1%\"...", section);
				InitAlgo(ini, section, tradeSystem, securities, algos, settings);
			}
		}

		foreach (auto a, algos) {
			dispatcher.Register(a);
		}

		Connect(*tradeSystem);
		Connect(marketDataSource);

		foreach (auto &a, algos) {
			a->SubscribeToMarketData(marketDataSource);
		}

		Log::Info("Loaded %1% securities.", securities.size());
		Log::Info("Loaded %1% strategies.", algos.size());

	}

	void UpdateSettingsRuntime(
				const fs::path &iniFilePath,
				Algos &algos,
				Settings &settings) {
		Log::Info("Detected INI-file %1% modification, updating current settings...", iniFilePath);
		const IniFile ini(iniFilePath);
		const std::list<std::string> sections = ini.ReadSectionsList();
		foreach (const auto &section, sections) {
			if (section == Ini::Sections::common) {
				settings.Update(ini, section);
				continue;
			}
			bool isError = false;
			foreach (boost::shared_ptr<Algo> &a, algos) {
				if (section == Ini::Sections::Algo::level2MarketArbitrage) {
					if (!dynamic_cast<Strategies::Level2MarketArbitrage::Algo *>(a.get())) {
						continue;
					}
				} else if (section == Ini::Sections::Algo::QuickArbitrage::old) {
					if (!dynamic_cast<Strategies::QuickArbitrage::Old *>(a.get())) {
						continue;
					}
				} else if (section == Ini::Sections::Algo::QuickArbitrage::askBid) {
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

	boost::shared_ptr<Settings> settings = LoadOptions(iniFilePath);
	boost::shared_ptr<TradeSystem> tradeSystem(new InteractiveBrokersTradeSystem);
	IqFeedClient marketDataSource;
	Dispatcher dispatcher(settings);
	Algos algos;
	InitTrading(iniFilePath, tradeSystem, dispatcher, marketDataSource, algos, settings);

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
