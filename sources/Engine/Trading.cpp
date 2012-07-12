/**************************************************************************
 *   Created: 2012/07/08 14:06:55
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Strategies/QuickArbitrage/QuickArbitrage.hpp"
#include "IqFeed/IqFeedClient.hpp"
#include "InteractiveBrokers/InteractiveBrokersTradeSystem.hpp"
#include "Dispatcher.hpp"
#include "Core/Security.hpp"

namespace pt = boost::posix_time;

namespace {

	typedef std::map<std::string, boost::shared_ptr<DynamicSecurity>> Securities;
	typedef std::list<boost::shared_ptr<Algo>> Algos;

	namespace Ini {
		namespace Sections {
			const std::string common = "Common";
			namespace Algo {
				const std::string quickArbitrage = "Algo.QuickArbitrageOld";
			}
		}
		namespace Key {
			const std::string symbols = "symbols";
		}
	}

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

}

void Trade(const std::string &iniFilePath) {

	boost::shared_ptr<TradeSystem> tradeSystem(new InteractiveBrokersTradeSystem);
	Dispatcher dispatcher;
	IqFeedClient marketDataSource;

	{

		Log::Info("Using %1% file...", iniFilePath);
		const IniFile ini(iniFilePath);

		const std::list<std::string> sections = ini.ReadSectionsList();

		foreach (const auto &section, sections) {
			if (section == Ini::Sections::common) {
				Log::Info("Found section \"%1%\", loading common options...", section);
				break;
			}
		}

		Securities securities;
		Algos algos;
		foreach (const auto &section, sections) {
			if (section == Ini::Sections::common) {
				continue;
			}
			Log::Info("Found section \"%1%\"...", section);
			if (section == Ini::Sections::Algo::quickArbitrage) {
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
						boost::shared_ptr<Algo> algo(
							new Strategies::QuickArbitrage::Algo(
								securities[CreateSecuritiesKey(symbol)],
								ini,
								section));
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
		}

		foreach (auto a, algos) {
			dispatcher.Register(a);
		}

		Connect(*tradeSystem);
	
		
		Connect(marketDataSource);
		foreach (auto &s, securities) {
			marketDataSource.SubscribeToMarketData(s.second);
		}

		Log::Info("Loaded %1% securities.", securities.size());
		Log::Info("Loaded %1% strategies.", algos.size());

	}

	dispatcher.Start();
	getchar();

}


