/**************************************************************************
 *   Created: 2013/02/02 21:02:42
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Server.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::EngineServer;

//////////////////////////////////////////////////////////////////////////

namespace {

	std::ofstream tradingLog;
	std::ofstream eventLog;

	fs::path InitLogFile(const fs::path &filePath, std::ofstream &file) {
		boost::filesystem::create_directories(filePath.branch_path());
		file.open(
			filePath.c_str(),
			std::ios::out | std::ios::ate | std::ios::app);
		if (!file) {
			std::cerr
				<< "Failed to open log file " << filePath << "."
				<< std::endl;
		}
		return filePath;
	}

	void InitLogs(
				bool useFile,
				bool useStdOut,
				bool useTrading,
				int argc,
				const char *argv[],
				const fs::path &logDir = fs::path()) {
		if (useFile) {
			InitLogFile(logDir / "event.log", eventLog);
			Log::EnableEvents(eventLog);
		}
		if (useStdOut) {
			Log::EnableEventsToStdOut();
		}
		{
			std::list<std::string> cmd;
			if (useFile) {
				for (auto i = 0; i < argc; ++i) {
					cmd.push_back(argv[i]);
				}
				Log::Info(
					"Started: \"%1%\"."
						" Build: " TRDK_BUILD_IDENTITY "."
						" Build time: " __TIME__ " " __DATE__ ".",
					boost::join(cmd, " "));
				Log::Info(
					"Local time: %1%.",
					boost::posix_time::microsec_clock::local_time());
			}
		}
		if (useTrading) {
			const auto filePath
				= InitLogFile(logDir / "trading.log", tradingLog);
			Log::EnableTrading(tradingLog);
			Log::Info("Logging trading to file %1%...", filePath);
		}
	}

	fs::path GetIniFilePath(const char *inputValue) {
		fs::path result = Normalize(GetExeWorkingDir() / inputValue);
		if (fs::is_directory(result)) {
			result /= "default.ini";
		}
		return result;
	}

}

//////////////////////////////////////////////////////////////////////////

namespace {

	bool RunServerService(int argc, const char *argv[]) {
		InitLogs(true, false, true, argc, argv);
		//! @todo Implement service installation
		return false;
	}

	bool InstallServerService(int /*argc*/, const char * /*argv*/[]) {
		//! @todo Implement service installation
		return false;
	}

	bool UninstallServerService(int /*argc*/, const char * /*argv*/[]) {
		//! @todo Implement service commands
		return false;
	}

	bool StartServerService(int /*argc*/, const char * /*argv*/[]) {
		//! @todo Implement service commands
		return false;
	}

	bool StopServerService(int /*argc*/, const char * /*argv*/[]) {
		//! @todo Implement service commands
		return false;
	}

	bool ShowServerServiceStatus(int /*argc*/, const char * /*argv*/[]) {
		//! @todo Implement service commands
		return false;
	}

	bool RunServerStandalone(int argc, const char *argv[]) {
		if (argc < 3 || !strlen(argv[2])) {
			std::cout << "No configuration file specified." << std::endl;
			return false;
		}
		const fs::path confFilePath = GetIniFilePath(argv[2]);
		{
			//! @todo Hardcoded INI-key name.
			const auto logsPath = IniFile(confFilePath)
				.ReadFileSystemPath("Common", "logs_dir");
			InitLogs(true, true, true, argc, argv, logsPath);
		}
		//! @todo Implement run standalone command
		return false;
	}

	bool DebugStrategy(int argc, const char *argv[]) {
	
		if (argc < 3 || !strlen(argv[2])) {
			std::cout << "No configuration file specified." << std::endl;
			return false;
		}

		const char *const uuid = "DEBUG";
	
		//! @todo read configuration from std in
		const fs::path confFilePath = GetIniFilePath(argv[2]);
		{
			IniFile ini(confFilePath);
			//! @todo Hardcoded INI-key name.
			auto logsPath = ini.ReadFileSystemPath("Common", "logs_dir");
			logsPath /= uuid;
			InitLogs(
				true,
				true,
				ini.ReadBoolKey("Common", "trading_log"),
				argc,
				argv,
				logsPath);
		}

		Server server;
		bool result = true;

		try {
			server.Run(uuid, confFilePath, false);
		} catch (const Exception &ex) {
			Log::Error("Failed to start engine: \"%1%\".", ex.what());
			result = false;
		}

		if (result) {
		
			std::cout
				<< std::endl
				<< "To stop please enter:" << std::endl
				<< "\t's' - normal stop,"
					<< " wait until all positions will be closed and stop;"
					<< std::endl
				<< "\t'u' - urgent stop,"
					<< " all position will be forcibly closed before;"
					<< std::endl
				<< std::endl;
			
			for ( ; ; ) {
				const auto &stopMode = getchar();
				if (stopMode == 's' || stopMode == 'S') {
					std::cout
						<< "Waiting until all positions will be closed"
							<< " by conditions..."
						<< " To interrupt closing process use Ctrl + C."
						<< std::endl;
					try {
						server.WaitForCancelAndStop();
					} catch (const Exception &ex) {
						Log::Error(
							"Failed to stop engine: \"%1%\".",
							ex.what());
						result = false;
					}
					break;
				} else if (stopMode == 'u' || stopMode == 'U') {
					std::cout
						<< "Closing all position before stop..."
						<< " To interrupt closing process use Ctrl + C."
						<< std::endl;
					try {
						server.CancelAllAndStop();
						break;
					} catch (const Exception &ex) {
						Log::Error(
							"Failed to stop engine: \"%1%\".",
							ex.what());
						result = false;
					}
					break;
				} else {
					std::cout
						<< "Unknown command."
							<< " Please enter 's' (normal stop)"
							<< " or 'u' (urgent)."
							<< std::endl;
				}
			}

		} else {
			getchar();
		}

		return result;

	}

}

//////////////////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {
	
	int result = 3;
	
	try {

		boost::function<bool (int, const char *[])> func;
		if (argc > 1) {
			Verify(--result >= 0);
			typedef std::map<std::string, decltype(func)> Commands;
			Commands commands;
			size_t i = 0;
			commands["service"] = &RunServerService; ++i;
			commands["install"] = &InstallServerService; ++i;
			commands["uninstall"] = &UninstallServerService; ++i;
			commands["start"] = &StartServerService; ++i;
			commands["stop"] = &StopServerService; ++i;
			commands["status"] = &ShowServerServiceStatus; ++i;
			commands["s"] = &ShowServerServiceStatus; ++i;
			commands["standalone"] = &RunServerStandalone; ++i;
			commands["r"] = &RunServerStandalone; ++i;
			commands["debug"] = &DebugStrategy; ++i;
			commands["d"] = &DebugStrategy; ++i;
			AssertEq(i, commands.size());
			const Commands::const_iterator commandPos = commands.find(argv[1]);
			if (commandPos != commands.end()) {
				Verify(--result >= 0);
				func = commandPos->second;
			} else {
				std::list<std::string> commandsStr; 
				foreach (const auto &cmd, commands) {
					if (cmd.first.size() > 1) {
						commandsStr.push_back(cmd.first);
					}
				}
				std::cerr
					<< "No command specified." << std::endl
					<< "Usage: " << argv[0]
					<< " [ " << boost::join(commandsStr, " ] | [ ") << " ]"
					<< std::endl
					<< std::endl
					<< "Debug:" << std::endl
					<< "    d, debug \"path to INI-file"
						<< " or path to default.ini directory\"" << std::endl
					<< std::endl
					<< std::endl;
			}
		}
		
		if (func) {
			if (func(argc, argv)) {
				Verify(--result >= 0);
			}
		}

	} catch (...) {
		AssertFailNoException();
	}
	Log::Info("Stopped.");
#	ifdef DEV_VER
		getchar();
#	endif
	return result;

}
