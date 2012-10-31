/**************************************************************************
 *   Created: 2012/07/09 16:02:36
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"

namespace fs = boost::filesystem;

void Trade(const fs::path &, bool isReplayMode);

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

	bool IsReplayMode(int argc, const char *argv[]) {
		for (auto i = 1; i < argc; ++i) {
			if (!_stricmp(argv[i], "replay")) {
				return true;
			}
		}
		return false;
	}

}

int main(int argc, const char *argv[]) {
	int result = 0;
	try {
		InitLogs(argc, argv);
		const fs::path iniFilePath = "Etc/trade.ini";
		Trade(iniFilePath, IsReplayMode(argc, argv));
	} catch (...) {
		result = 1;
		AssertFailNoException();
	}
	Log::Info("Stopped.");
#	ifdef DEV_VER
		getchar();
#	endif
	return result;
}
