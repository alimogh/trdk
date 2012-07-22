/**************************************************************************
 *   Created: 2012/07/09 16:02:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"

namespace fs = boost::filesystem;

void Trade(const fs::path &);
void RequestMarketData(const fs::path &, int argc, const char *argv[]);

namespace {

	std::ofstream tradingLog;
	std::ofstream eventsLog;

	fs::path InitLogFile(const std::string &fileName, std::ofstream &file) {
		fs::path path = Defaults::GetLogFilePath();
		path /= fileName;
		boost::filesystem::create_directories(path.branch_path());
		file.open(
			path.c_str(),
			std::ios::out | std::ios::ate | std::ios::app);
		if (!file) {
			std::cerr << "Failed to open log file " << path << "." << std::endl;
		}
		return path;
	}

	void InitLogs(int argc, const char *argv[]) {
		{
			InitLogFile("events.log", eventsLog);
			Log::EnableEvents(eventsLog);
			std::list<std::string> cmd;
			for (auto i = 0; i < argc; ++i) {
				cmd.push_back(argv[i]);
			}
			Log::Info("Command: \"%1%\".", boost::join(cmd, " "));
		}
		{
			const auto filePath = InitLogFile("trading.log", tradingLog);
			Log::EnableTrading(tradingLog);
			Log::Info("Logging trading to file %1%...", filePath);
		}
	}

}

void main(int argc, const char *argv[]) {
	try {
		InitLogs(argc, argv);
		const fs::path iniFilePath = "Etc/trade.ini";
		if (argc >= 2 && std::string(argv[1]) == "market-data") {
			RequestMarketData(iniFilePath, argc, argv);
		} else {
			Trade(iniFilePath);
		}
	} catch (const std::exception &ex) {
		Log::Error("Unexpected error: \"%1%\".", ex.what());
	} catch (...) {
		AssertFailNoException();
	}
	Log::Info("Stopped.");
#	ifdef DEV_VER
		getchar();
#	endif
}
