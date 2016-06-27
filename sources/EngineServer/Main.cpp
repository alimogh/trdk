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
#include "Engine.hpp"
#include "Service.hpp"

namespace fs = boost::filesystem;

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

	bool RunServerStandalone(int argc, const char *argv[]) {
		
		if (argc < 3 || !strlen(argv[2])) {
			std::cerr << "No configuration file specified." << std::endl;
			getchar();
			return false;
		}

		try {
			Service service(GetIniFilePath(argv[2]));
			getchar();
			return true;
		} catch (const trdk::Lib::Exception &ex) {
			std::cerr
				<< "Failed to start engine service: \""
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

		std::unique_ptr<Engine> engine;
		bool result = true;

		{
		
			std::vector<std::string> cmd;
			for (auto i = 0; i < argc; ++i) {
				cmd.push_back(argv[i]);
			}

			try {
				engine.reset(
					new Engine(
						GetIniFilePath(argv[2]),
						[](const trdk::Context::State &, const std::string *) {
							//...//
						},
						true));
			} catch (const trdk::Lib::Exception &ex) {
				std::cerr
					<< "Failed to start engine: \"" << ex << "\"."
					<< std::endl;
				result = false;
			}

		}
		
		if (result) {

			trdk::StopMode stopMode = trdk::STOP_MODE_UNKNOWN;
			std::cout << std::endl;
			bool skipInfo = false;
			while (stopMode == trdk::STOP_MODE_UNKNOWN) {
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
						stopMode = trdk::STOP_MODE_GRACEFULLY_POSITIONS;
						break;
					case '2':
						stopMode = trdk::STOP_MODE_GRACEFULLY_ORDERS;
						break;
					case '5':
						engine->ClosePositions();
						break;
					case '9':
					case 'u':
					case 'U':
						stopMode = trdk::STOP_MODE_IMMEDIATELY;
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
				engine->Stop(stopMode);
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
