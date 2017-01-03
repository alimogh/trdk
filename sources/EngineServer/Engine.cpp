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
#include "Engine.hpp"
#include "Exception.hpp"
#include "Core/Settings.hpp"
#include "Common/VersionInfo.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

using namespace trdk::EngineServer;
using namespace trdk::Lib;

Engine::Engine(
		const fs::path &path,
		const trdk::Engine::Context::StateUpdateSlot &contextStateUpdateSlot,
		bool enableStdOutLog,
		const boost::unordered_map<std::string, std::string> &params) {
	Run(path, contextStateUpdateSlot, nullptr, enableStdOutLog, params);
}

Engine::Engine(
		const fs::path &path,
		const trdk::Engine::Context::StateUpdateSlot &contextStateUpdateSlot,
		trdk::DropCopy &dropCopy,
		bool enableStdOutLog,
		const boost::unordered_map<std::string, std::string> &params) {
	Run(path, contextStateUpdateSlot, &dropCopy, enableStdOutLog, params);
}

Engine::~Engine() {
	if (m_context) {
		Stop(STOP_MODE_IMMEDIATELY);
	}
}

void Engine::Run(
		const fs::path &path,
		const trdk::Engine::Context::StateUpdateSlot &contextStateUpdateSlot,
		trdk::DropCopy *dropCopy,
		bool enableStdOutLog,
		const boost::unordered_map<std::string, std::string> &params) {

	Assert(!m_context);
	Assert(!m_eventsLog);
	Assert(!m_tradingLog);

	try {

		const IniFile ini(path);
		const trdk::Settings settings(
			ini,
			pt::microsec_clock::universal_time());

		fs::create_directories(settings.GetLogsInstanceDir());

		fs::copy_file(
			path,
			settings.GetLogsInstanceDir() / path.filename(),
			fs::copy_option::fail_if_exists);

		m_eventsLog = boost::make_unique<trdk::Engine::Context::Log>(
			settings.GetTimeZone());
		if (enableStdOutLog) {
			m_eventsLog->EnableStdOut();
		}
		{
			const auto &logFilePath
				= settings.GetLogsInstanceDir() / "engine.log";
			m_eventsLogFile.open(
				logFilePath.string().c_str(),
				std::ios::out | std::ios::ate | std::ios::app);
			if (!m_eventsLogFile) {
				boost::format error("Failed to open events log file %1%.");
				error % logFilePath;
				throw EngineServer::Exception(error.str().c_str());
			}
			m_eventsLog->EnableStream(m_eventsLogFile, true);
		}
		settings.Log(*m_eventsLog);

		VerifyModules();

		m_tradingLog = boost::make_unique<trdk::Engine::Context::TradingLog>(
			settings.GetTimeZone());

		m_context = boost::make_unique<trdk::Engine::Context>(
			*m_eventsLog,
			*m_tradingLog,
			settings,
			ini,
			params);

		m_context->SubscribeToStateUpdates(contextStateUpdateSlot);

		{
			const auto &tradingLogFilePath
				= settings.GetLogsInstanceDir() / "trading.log";
			if (ini.ReadBoolKey("General", "trading_log")) {
				m_tradingLogFile.open(
					tradingLogFilePath.string().c_str(),
					std::ios::out | std::ios::ate | std::ios::app);
				if (!m_tradingLogFile) {
					boost::format error(
						"Failed to open trading log file %1%.");
					error % tradingLogFilePath;
					throw EngineServer::Exception(error.str().c_str());
				}
				m_eventsLog->Info("Trading log: %1%.", tradingLogFilePath);
				m_tradingLog->EnableStream(
					m_tradingLogFile,
					*m_context);
			} else {
				m_eventsLog->Info("Trading log: DISABLED.");
			}
		}

		m_context->Start(ini, dropCopy);

	} catch (const trdk::Lib::Exception &ex) {
		if (m_eventsLog) {
			m_eventsLog->Error(
				"Failed to init engine context: \"%1%\".",
				ex.what());
		}
		boost::format message("Failed to init engine context: \"%1%\"");
		message % ex.what();
		throw Exception(message.str().c_str());
	}

}

trdk::Context & Engine::GetContext() {
	if (!m_context) {
		throw Exception("Engine has no context as it not started");
	}
	return *m_context;
}

void Engine::Stop(const trdk::StopMode &stopMode) {

	if (!m_context) {
		throw Exception("Failed to stop engine, engine is stopped");
	}

	boost::thread stopThread(
		[this, &stopMode]() {
			try {
				m_context->Stop(stopMode);
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		});
	stopThread.join();

	m_context.reset();

}

void Engine::ClosePositions() {
	if (!m_context) {
		throw Exception("Failed to close engine poisitions, engine is stopped");
	}
	m_context->ClosePositions();
}

void Engine::VerifyModules() const {
	
	boost::unordered_map<std::string, bool /* is required */> moduleList;
	{
		const std::string fullModuleList[]
			= TRDK_GET_MODUE_FILE_NAME_LIST();
		for (const auto &module: fullModuleList) {
			Assert(moduleList.count(module) == 0);
			moduleList[module] = false;
		}
	}
	{
		const std::string requiredModuleList[]
			= TRDK_GET_REQUIRED_MODUE_FILE_NAME_LIST();
		for (const auto &module: requiredModuleList) {
			AssertEq(1, moduleList.count(module));
			moduleList[module] = true;
		}
	}
	
	for (const auto &module: moduleList) {

		try {
			
			Dll dll(module.first, true);
		
			const auto getVerInfo
				= dll.GetFunction<void(VersionInfoV1 *)>(
					"GetTrdkModuleVersionInfoV1");
		
			VersionInfoV1 realModuleVersion;
			getVerInfo(&realModuleVersion);
			const VersionInfoV1 expectedModuleVersion(module.first);
			if (realModuleVersion != expectedModuleVersion) {
				m_eventsLog->Error(
					"Module %1% has wrong version"
						": \"%2%\", but must be \"%3%\".",
					dll.GetFile(),
					realModuleVersion,
					expectedModuleVersion);
				throw EngineServer::Exception("Module has wrong version");
			}

			m_eventsLog->Debug("Found module %1%.", dll.GetFile());
		
		} catch (const Dll::Error &ex) {
			
			if (!module.second) {
				continue;
			}
			
			m_eventsLog->Error(
				"Failed to verify the version of the required module \"%1%\""
					": \"%2%\".",
				module.first,
				ex.what());
			throw EngineServer::Exception(
				"Failed to verify the version of the required module");
		
		}
	
	}

}
