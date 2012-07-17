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
				Securities &securitites) {
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
					true));
			Log::Info("Loaded security \"%1%\".", securititesTmp[key]->GetFullSymbol());
		}
		securititesTmp.swap(securitites);
	}

	boost::shared_ptr<Settings> LoadOptions(const std::string &iniFilePath) {
		Log::Info("Using %1% file for common options...", iniFilePath);
		return boost::shared_ptr<Settings>(new Settings(IniFile(iniFilePath), Ini::Sections::common));
	}

	void InitAlgo(
				const IniFile &ini,
				const std::string &section,
				boost::shared_ptr<TradeSystem> tradeSystem,
				Securities &securities,
				Algos &algos)  {

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
			LoadSecurities(symbols, tradeSystem, securities);
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
				const std::string &iniFilePath,
				boost::shared_ptr<TradeSystem> tradeSystem,
				Dispatcher &dispatcher,
				IqFeedClient &marketDataSource)  {

		Log::Info("Using %1% file for algo options...", iniFilePath);
		const IniFile ini(iniFilePath);
		const std::list<std::string> sections = ini.ReadSectionsList();

		Securities securities;
		Algos algos;
		foreach (const auto &section, sections) {
			if (section != Ini::Sections::common) {
				Log::Info("Found section \"%1%\"...", section);
				InitAlgo(ini, section, tradeSystem, securities, algos);
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

}

void Trade(const std::string &iniFilePath) {

	boost::shared_ptr<Settings> options = LoadOptions(iniFilePath);
	boost::shared_ptr<TradeSystem> tradeSystem(new InteractiveBrokersTradeSystem);
	IqFeedClient marketDataSource;
	Dispatcher dispatcher(options);
	InitTrading(iniFilePath, tradeSystem, dispatcher, marketDataSource);

	dispatcher.Start();
	getchar();
	dispatcher.Stop();

}
