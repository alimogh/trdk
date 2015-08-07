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
#include "Service.hpp"

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
		} else if (!result.has_extension()) {
			result.replace_extension("ini");
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
			getchar();
			return false;
		}

		try {

			EngineServer::Service service(
				boost::asio::ip::host_name(),
				GetIniFilePath(argv[2]));

			getchar();

			return true;

		} catch (const trdk::Lib::Exception &ex) {
			std::cerr
				<< "Failed to start server service: \""
				<< ex << "\"." << std::endl;
			getchar();
		}

		return false;
	
	}

	bool DebugStrategy(int argc, const char *argv[]) {
	
		if (argc < 3 || !strlen(argv[2])) {
			std::cerr << "No configuration file specified." << std::endl;
			return false;
		}

		Server server;
		bool result = true;
		boost::signals2::signal<FooSlotSignature> fooSlotConnection;

		{
		
			std::vector<std::string> cmd;
			for (auto i = 0; i < argc; ++i) {
				cmd.push_back(argv[i]);
			}

			try {
				server.Run(
					fooSlotConnection,
					"__DEBUG",
					GetIniFilePath(argv[2]),
					true,
					boost::join(cmd, " "));
			} catch (const trdk::Lib::Exception &ex) {
				std::cerr
					<< "Failed to start engine: \"" << ex << "\"."
					<< std::endl;
				result = false;
			}

		}
		
		if (result) {

			StopMode stopMode = STOP_MODE_UNKNOWN;
			std::cout << std::endl;
			bool skipInfo = false;
			while (stopMode == STOP_MODE_UNKNOWN) {
				if (!skipInfo) {
					std::cout
						<< "To please enter:" << std::endl
							<< "\t1 - normal stop,"
								<< " wait until all positions will be completed;"
								<< std::endl
							<< "\t2 - gracefully stop,"
								<< " wait for current orders only;"
								<< std::endl
							<< "\t5 - close all positions;" << std::endl
							<< "\t9 - urgent stop, immediately." << std::endl
						<< std::endl;
				} else {
					skipInfo = false;
				}
				const char command = char(getchar());
				switch (command) {
					case '1':
						stopMode = STOP_MODE_GRACEFULLY_POSITIONS;
						break;
					case '2':
						stopMode = STOP_MODE_GRACEFULLY_ORDERS;
						break;
					case '5':
						server.ClosePositions();
						break;
					case '9':
					case 'u':
					case 'U':
						stopMode = STOP_MODE_IMMEDIATELY;
						break;
					case  '\n':
						skipInfo = true;
						break;
					default:
						std::cout << "Unknown command." << std::endl;
						break;
				}
			}

			try {
				server.StopAll(stopMode);
			} catch (const trdk::Lib::Exception &ex) {
				std::cerr
					<< "Failed to stop engine: \"" << ex << "\"."
					<< std::endl;
				result = false;
			}

		}

		std::cout << "Stopped. Press any key to exit." << std::endl;
		getchar();

		return result;

	}

	bool ShowVersion(int /*argc*/, const char * /*argv*/[]) {
		std::cout << TRDK_NAME " " TRDK_BUILD_IDENTITY << std::endl;
		return true;
	}

}

//////////////////////////////////////////////////////////////////////////

#if !defined(BOOST_WINDOWS)
namespace {

	void * WaitOsSignal(void *arg) {
		try {
			for (; ;) {
				sigset_t signalSet;
				int signalNumber = 0;
				sigwait(&signalSet, &signalNumber);
				std::cout
					<< "OS Signal " << signalNumber
					<< " received and suppressed." << std::endl;
			}
		} catch (...) {
			AssertFailNoException();
			throw;
		}
	}

	void InstallOsSignalsHandler() {

		// Start by masking suppressed signals at the primary thread. All other
		// threads inherit this signal mask and therefore will have the same
		// signals suppressed.
		{
			sigset_t signalSet;
			sigemptyset(&signalSet);
			sigaddset(&signalSet, SIGPIPE);
			// sigaddset(&signalSet, SIGXXX1);
			// sigaddset(&signalSet, SIGXXX2);
		}

		// Install the signal mask against primary thread.
		{
			sigset_t signalSet;
			const auto status = pthread_sigmask(SIG_BLOCK, &signalSet, NULL);
			if (status != 0) {
				std::cerr
					<< "Failed to install OS Signal Mask"
					<< " (error: " << status << ")."
					<< std::endl;
				exit(1);
			}
		}

		// Create the sigwait thread.
		{
			pthread_t signalThreadId;
			const auto status
				= pthread_create(&signalThreadId, NULL, WaitOsSignal, NULL);
			if (status != 0) {
				std::cerr
					<< "Failed to install create thread for OS Signal Wait"
					<< " (error: " << status << ")."
					<< std::endl;
				exit(1);
			}
		}

	}

}
#else
namespace {

	void InstallOsSignalsHandler() {
		//...//
	}

}
#endif

//////////////////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {
	
	int result = 3;
	
	try {

		InstallOsSignalsHandler();

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
			commands["version"] = &ShowVersion; ++i;
			commands["v"] = &ShowVersion; ++i;
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

	return result;

}
