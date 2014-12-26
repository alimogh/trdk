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
#include "Core/Settings.hpp"

namespace fs = boost::filesystem;

using namespace trdk;
using namespace trdk::EngineServer;
using namespace trdk::Lib;

Server::Server() {
	//..//
}

void Server::Run(
			const std::string &uuid,
			const fs::path &path,
			bool enableStdOutLog,
			int argc,
			const char *argv[]) {
	
	const Lock lock(m_mutex);
	
	{
		const auto &index = m_engines.get<ByUuid>();
		if (index.find(uuid) != index.end()) {
			boost::format message("Engine with UUID \"%1%\" already activated");
			message % uuid;
			throw Exception(message.str().c_str());
		}
	}

	boost::shared_ptr<IniFile> ini(new IniFile(path));

	EngineInfo info = {};
	info.uuid = uuid;
	info.eventsLog.reset(new Engine::Context::Log);
	if (enableStdOutLog) {
		info.eventsLog->EnableStdOut();
	}
	info.tradingLog.reset(new Engine::Context::TradingLog);

	Settings settings(
		ini->ReadBoolKey("Common", "is_replay_mode"),
		ini->ReadFileSystemPath("Common", "logs_dir") / uuid);

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
			throw Exception(error.str().c_str());
		}
		info.eventsLog->EnableStream(*info.eventsLogFile, true);
	}
	{
		std::vector<std::string> cmd;
		for (auto i = 0; i < argc; ++i) {
			cmd.push_back(argv[i]);
		}
		info.eventsLog->Info("Command: \"%1%\".", boost::join(cmd, " "));
		if (settings.IsReplayMode()) {
			info.eventsLog->Warn("Replay mode.");
		}
	}

	settings.Update(*ini, *info.eventsLog);

	info.engine.reset(
		new Engine::Context(
			*info.eventsLog,
			*info.tradingLog,
			settings,
			ini));

	{
		const auto &tradingLogFilePath = settings.GetLogsDir() / "trading.log";
		if (ini->ReadBoolKey("Common", "trading_log")) {
			info.tradingLogFile.reset(
				new std::ofstream(
					tradingLogFilePath.string().c_str(),
					std::ios::out | std::ios::ate | std::ios::app));
			if (!*info.tradingLogFile) {
				boost::format error("Failed to open trading log file %1%.");
				error % tradingLogFilePath;
				throw Exception(error.str().c_str());
			}
			info.eventsLog->Info("Trading log: %1%.", tradingLogFilePath);
			info.tradingLog->EnableStream(*info.tradingLogFile, *info.engine);
		} else {
			info.eventsLog->Info("Trading log: DISABLED.");
		}
	}

	info.engine->Start();

	m_engines.insert(info);

}

void Server::CancelAllAndStop() {
	{
		Lock lock(m_mutex);
		foreach (const auto &e, m_engines) {
			e.engine->CancelAllAndBlock();
		}
	}
	StopAll();
}

void Server::WaitForCancelAndStop() {
	{
		Lock lock(m_mutex);
		foreach (const auto &e, m_engines) {
			e.engine->WaitForCancelAndBlock();
		}
	}
	StopAll();
}

void Server::StopAll() {
	const Lock lock(m_mutex);
	Engines().swap(m_engines);
}
