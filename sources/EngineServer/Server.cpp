/**************************************************************************
 *   Created: 2013/02/08 12:13:44
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Server.hpp"
#include "Exception.hpp"
#include "Core/Settings.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::EngineServer;
using namespace trdk::Lib;

Server::Server() {
	//..//
}

bool Server::IsStarted(const std::string &id) const {
	const Lock lock(m_mutex);
	return m_engines.get<ById>().find(id) != m_engines.get<ById>().end();
}

void Server::Run(
		boost::signals2::signal<FooSlotSignature> &fooSlotConnection,
		const std::string &id,
		const fs::path &path,
		bool enableStdOutLog,
		const std::string &commandInfo) {

	if (IsStarted(id)) {
		boost::format message("Engine with ID \"%1%\" already started");
		message % id;
		throw EngineServer::Exception(message.str().c_str());
	}

	EngineInfo info = {};

	try {

		info.id = id;
		info.eventsLog.reset(new Engine::Context::Log);
		const auto &startTime = info.eventsLog->GetTime();

		boost::shared_ptr<IniFile> ini(new IniFile(path));

		if (enableStdOutLog) {
			info.eventsLog->EnableStdOut();
		}
		info.tradingLog.reset(new Engine::Context::TradingLog);

		Settings settings(
			ini->ReadBoolKey("Common", "is_replay_mode"),
			ini->ReadFileSystemPath("Common", "logs_dir") / id);

		fs::create_directories(settings.GetLogsDir());
		{
			const auto &logFilePath = settings.GetLogsDir() / "event.log";
			info.eventsLogFile.reset(
				new std::ofstream(
					logFilePath.string().c_str(),
					std::ios::out | std::ios::ate | std::ios::app));
			if (!*info.eventsLogFile) {
				boost::format error("Failed to open events log file %1%.");
				error % logFilePath;
				throw EngineServer::Exception(error.str().c_str());
			}
			info.eventsLog->EnableStream(*info.eventsLogFile, true);
		}

		info.eventsLog->Info("Engine ID: \"%1%\".", id);
		info.eventsLog->Info("Command: \"%1%\".", commandInfo);
		if (settings.IsReplayMode()) {
			info.eventsLog->Warn("Replay mode.");
		}
		
		settings.Update(*ini, *info.eventsLog);

		info.engine.reset(
			new Engine::Context(
				fooSlotConnection,
				*info.eventsLog,
				*info.tradingLog,
				settings,
				startTime,
				ini));

		{
			const auto &tradingLogFilePath
				= settings.GetLogsDir() / "trading.log";
			if (ini->ReadBoolKey("Common", "trading_log")) {
				info.tradingLogFile.reset(
					new std::ofstream(
						tradingLogFilePath.string().c_str(),
						std::ios::out | std::ios::ate | std::ios::app));
				if (!*info.tradingLogFile) {
					boost::format error("Failed to open trading log file %1%.");
					error % tradingLogFilePath;
					throw EngineServer::Exception(error.str().c_str());
				}
				info.eventsLog->Info("Trading log: %1%.", tradingLogFilePath);
				info.tradingLog->EnableStream(
					*info.tradingLogFile,
					*info.engine);
			} else {
				info.eventsLog->Info("Trading log: DISABLED.");
			}
		}

		info.engine->Start();

		const Lock lock(m_mutex);
		m_engines.insert(info);

	} catch (const trdk::Lib::Exception &ex) {
		if (info.eventsLog) {
			info.eventsLog->Warn(
				"Failed to init engine context: \"%1%\".",
				ex.what());
		}
		boost::format message("Failed to init engine context: \"%1%\"");
		message % ex.what();
		throw EngineServer::Exception(message.str().c_str());
	}

}

void Server::StopAll(const trdk::StopMode &stopMode) {

	const Lock lock(m_mutex);

	{
		boost::thread_group stopThreads;
		foreach (const EngineInfo &engineInfo, m_engines) {
			stopThreads.create_thread(
				[&]() {
					const_cast<Engine::Context &>(*engineInfo.engine)
						.Stop(stopMode);
				});
		}
		stopThreads.join_all();
	}

	Engines().swap(m_engines);

}
