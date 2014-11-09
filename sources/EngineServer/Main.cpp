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

	bool RunServerService(int /*argc*/, const char * /*argv*/[]) {
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
			std::cerr << "No configuration file specified." << std::endl;
			return false;
		}
		//! @todo Implement run standalone command
		return false;
	}

	bool DebugStrategy(int argc, const char *argv[]) {
	
		if (argc < 3 || !strlen(argv[2])) {
			std::cerr << "No configuration file specified." << std::endl;
			return false;
		}

		const char *const uuid = "DEBUG";
	
		Server server;
		bool result = true;

		try {
			server.Run(uuid, GetIniFilePath(argv[2]), false, true, argc, argv);
		} catch (const Exception &ex) {
			std::cerr
				<< "Failed to start engine: \"" << ex << "\"."
				<< std::endl;
			result = false;
		}

		getchar();
		
		if (result) {
			try {
				server.StopAll();
			} catch (const Exception &ex) {
				std::cerr
					<< "Failed to stop engine: \"" << ex << "\"."
					<< std::endl;
				result = false;
				getchar();
			}
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
#	ifdef DEV_VER
		getchar();
#	endif
	return result;

}
