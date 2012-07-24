/**************************************************************************
 *   Created: 2012/07/22 22:06:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Ini.hpp"
#include "Util.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "IqFeed/IqFeedClient.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace lt = boost::local_time;

void RequestMarketData(const fs::path &iniFilePath, int argc, const char *argv[]) {

	Log::Info("Starting Market Data request...");

	Assert(argc >= 2 && std::string(argv[1]) == "market-data");

	if (argc != 3) {
		throw Exception("Failed to get request parameters (request data)");
	}

	IqFeedClient marketDataSource;

	std::map<std::string, boost::shared_ptr<Security>> securities;
	{

		const IniFile ini(iniFilePath);

		const boost::shared_ptr<const Settings> settings = Ini::LoadSettings(
			iniFilePath,
			pt::time_from_string((boost::format("%1% 00:00:00") % argv[2]).str()),
			false);

		Log::Info(
			"Market data request period: %1% - %2%.",
			settings->GetCurrentTradeSessionStartTime() + Util::GetEdtDiff(),
			settings->GetCurrentTradeSessionEndime() + Util::GetEdtDiff());

		Connect(marketDataSource);
		
		const fs::path symbolsFilePath = ini.ReadKey(
			Ini::Sections::MarketData::request,
			Ini::Key::symbols,
			false);
		Log::Info("Loading symbols from %1%...", symbolsFilePath);
		foreach (
				const IniFile::Symbol &symbol,
				IniFile(symbolsFilePath, ini.GetPath().branch_path()).ReadSymbols("SMART", "NASDAQ")) {
			const std::string key
				= Util::CreateSymbolFullStr(symbol.symbol, symbol.primaryExchange, symbol.exchange);
			if (securities.find(key) != securities.end()) {
				Log::Warn("Symbol %1% not unique in the %2%.", symbol.symbol, iniFilePath);
				continue;
			}
			securities[key] = boost::shared_ptr<Security>(
				new Security(
					symbol.symbol,
					symbol.primaryExchange,
					symbol.exchange,
					settings,
					true));
			marketDataSource.RequestHistory(
				securities[key],
				settings->GetCurrentTradeSessionStartTime(),
				settings->GetCurrentTradeSessionEndime());
			Log::Info("Loaded security \"%1%\".", securities[key]->GetFullSymbol());
		}
		Log::Info("Loaded %1% securities.", securities.size());
	
	}

	boost::uint64_t i = 0;
	for ( ; ; ++i) {
		const auto sleepTime = i < 12
			?	pt::seconds(5)
			:	i < 18
				?	pt::seconds(10)
				:	pt::seconds(15);
		boost::this_thread::sleep(boost::get_system_time() + sleepTime);
		bool isCompleted = true;
		foreach (const auto s, securities) {
			if (s.second->IsHistoryData())  {
				isCompleted = false;
				break;
			}
		}
		if (isCompleted) {
			break;
		}
	}

	Log::Info("Finished.");

}
