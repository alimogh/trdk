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

using namespace trdk::Lib;
using namespace trdk::EngineServer;

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

////////////////////////////////////////////////////////////////////////////////

namespace trdk { namespace EngineServer {

	namespace CommandLine {

		namespace Commands {
	
			const char *const debug = "debug";
			const char *const debugShort = "d";

			const char *const standalone = "standalone";
			const char *const standaloneShort = "s";

			const char *const version = "version";
			const char *const versionShort = "v";
			
			const char *const help = "help";
			const char *const helpEx = "--help";
			const char *const helpShort = "h";
			const char *const helpShortEx = "-h";

		}

		namespace Options {
			const char *const startDelay = "--start_delay";
		}

	}

} }

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

	bool RunService(int argc, const char *argv[]) {
		
		if (argc < 3 || !strlen(argv[2])) {
			std::cerr << "No configuration file specified." << std::endl;
			return false;
		}

		pt::time_duration startDelay;
		if (argc > 3) {
			auto i = 3;
			if (strcmp(argv[i], CommandLine::Options::startDelay)) {
				std::cerr
					<< "Unknown option \"" << argv[i] << "\"."
					<< std::endl;
				return false;
			}
			const auto valueArgIndex = i + 1;
			if (valueArgIndex >= argc) {
				std::cerr
					<< "Option " << CommandLine::Options::startDelay
					<< " has no value." << std::endl;
				return false;
			}
			try {
				const auto value
					= boost::lexical_cast<unsigned short>(argv[valueArgIndex]);
				startDelay = pt::seconds(value);
			} catch (const boost::bad_lexical_cast &ex) {
				std::cerr
					<< "Failed to read " << CommandLine::Options::startDelay
					<< " value \"" << argv[valueArgIndex] << "\":"
					<< " \"" << ex.what() << "\"." << std::endl;
				return false;
			}
		}

		try {
			Service service(GetIniFilePath(argv[2]), startDelay);
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
		} else if (argc > 3) {
			std::cerr << "Unknown option \"" << argv[3] << "\"." << std::endl;
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

	bool ShowHelp(int argc, const char *argv[]) {

		using namespace trdk::EngineServer::CommandLine::Commands;
		using namespace trdk::EngineServer::CommandLine::Options;

		std::cout << std::endl;

		if (!ShowVersion(argc, argv)) {
			return false;
		}

		std::cout
			<< std::endl
			<< "Usage: "
				<< argv[0]
				<< " command command-args  [ --options [options-args] ]"
				<< std::endl
			<< std::endl
			<< "Command:" << std::endl
			<< std::endl
				<< "    " << standalone << " (or " << standaloneShort << ")"
				" \"path to INI-file or path to default.ini directory\""
				<< std::endl
				<< std::endl
				<< "    " << debug << " (or " << debugShort << ")"
				<< " \"path to INI-file or path to default.ini directory\""
				<< std::endl
				<< std::endl
				<< "    " << help << " (or " << helpShort << ")" << std::endl
			<< std::endl
			<< "Options:" << std::endl
			<< std::endl
				<< "    " << startDelay << " \"number of seconds to wait before"
				<< " service start\"" << std::endl
			<< std::endl
			<< std::endl;

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
			
			using namespace trdk::EngineServer::CommandLine::Commands;
			using namespace trdk::EngineServer::CommandLine::Options;
			
			Verify(--result >= 0);

			boost::unordered_map<std::string, decltype(func)> commands;

			Verify(
				commands.emplace(
						std::make_pair(standalone, &RunService))
					.second);
			Verify(
				commands.emplace(
						std::make_pair(standaloneShort, &RunService))
					.second);
			
			Verify(
				commands.emplace(std::make_pair(debug, &DebugStrategy)).second);
			Verify(
				commands.emplace(std::make_pair(debugShort, &DebugStrategy))
					.second);
			
			Verify(
				commands.emplace(std::make_pair(version, &ShowVersion)).second);
			Verify(
				commands.emplace(std::make_pair(versionShort, &ShowVersion))
					.second);

			Verify(
				commands.emplace(std::make_pair(help, &ShowHelp)).second);
			Verify(
				commands.emplace(std::make_pair(helpShort, &ShowHelp))
					.second);
			Verify(
				commands.emplace(std::make_pair(helpEx, &ShowHelp)).second);
			Verify(
				commands.emplace(std::make_pair(helpShortEx, &ShowHelp))
					.second);

			const auto &commandPos = commands.find(argv[1]);
			if (commandPos != commands.cend()) {
				Verify(--result >= 0);
				func = commandPos->second;
			} else {
				std::cerr << "No command specified." << std::endl;
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
