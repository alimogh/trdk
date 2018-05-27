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
#include "Core/Settings.hpp"
#include "Core/TradingLog.hpp"
#include "Common/VerifyModules.hpp"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

using namespace trdk::Engine;
using namespace trdk::Lib;

class Engine::Implementation {
 public:
  std::ofstream m_eventsLogFile;
  std::unique_ptr<Context::Log> m_eventsLog;

  std::ofstream m_tradingLogFile;
  std::unique_ptr<Context::TradingLog> m_tradingLog;

  std::unique_ptr<Context> m_context;

  Implementation() = default;
  Implementation(const Implementation&) = delete;
  Implementation& operator=(const Implementation&) = delete;
};

Engine::Engine(
    const fs::path& configFile,
    const fs::path& logsDir,
    const Context::StateUpdateSlot& contextStateUpdateSlot,
    const boost::function<void(const std::string&)>& startProgressCallback,
    const boost::function<bool(const std::string&)>& startErrorCallback,
    const boost::function<void(Context::Log&)>& logStartCallback)
    : m_pimpl(boost::make_unique<Implementation>()) {
  Run(configFile, logsDir, contextStateUpdateSlot, nullptr,
      startProgressCallback, startErrorCallback, logStartCallback);
}

Engine::Engine(
    const fs::path& configFile,
    const fs::path& logsDir,
    const Context::StateUpdateSlot& contextStateUpdateSlot,
    trdk::DropCopy& dropCopy,
    const boost::function<void(const std::string&)>& startProgressCallback,
    const boost::function<bool(const std::string&)>& startErrorCallback,
    const boost::function<void(Context::Log&)>& logStartCallback)
    : m_pimpl(boost::make_unique<Implementation>()) {
  Run(configFile, logsDir, contextStateUpdateSlot, &dropCopy,
      startProgressCallback, startErrorCallback, logStartCallback);
}

Engine::Engine(Engine&&) = default;

Engine::~Engine() {
  if (m_pimpl->m_context) {
    Stop(STOP_MODE_IMMEDIATELY);
  }
}

void Engine::Run(
    const fs::path& configFile,
    const fs::path& logsDir,
    const Context::StateUpdateSlot& contextStateUpdateSlot,
    trdk::DropCopy* dropCopy,
    const boost::function<void(const std::string&)>& startProgressCallback,
    const boost::function<bool(const std::string&)>& startErrorCallback,
    const boost::function<void(Context::Log&)>& logStartCallback) {
  Assert(!m_pimpl->m_context);
  Assert(!m_pimpl->m_eventsLog);
  Assert(!m_pimpl->m_tradingLog);

  try {
    if (startProgressCallback) {
      startProgressCallback("Reading settings...");
    }

    {
      Settings settings(configFile, logsDir,
                        pt::microsec_clock::universal_time());

      create_directories(settings.GetLogsDir());

      copy_file(configFile, settings.GetLogsDir() / configFile.filename(),
                fs::copy_option::fail_if_exists);

      m_pimpl->m_eventsLog =
          boost::make_unique<Context::Log>(settings.GetTimeZone());
      if (logStartCallback) {
        logStartCallback(*m_pimpl->m_eventsLog);
      }
      {
        const auto& logFilePath = settings.GetLogsDir() / "engine.log";
        m_pimpl->m_eventsLogFile.open(
            logFilePath.string().c_str(),
            std::ios::out | std::ios::ate | std::ios::app);
        if (!m_pimpl->m_eventsLogFile) {
          boost::format error("Failed to open events log file %1%.");
          error % logFilePath;
          throw Exception(error.str().c_str());
        }
        m_pimpl->m_eventsLog->EnableStream(m_pimpl->m_eventsLogFile, true);
      }
      settings.Log(*m_pimpl->m_eventsLog);

      if (startProgressCallback) {
        startProgressCallback("Verifying modules...");
      }
      VerifyModules();

      if (startProgressCallback) {
        startProgressCallback("Starting engine...");
      }

      m_pimpl->m_tradingLog =
          boost::make_unique<Context::TradingLog>(settings.GetTimeZone());

      m_pimpl->m_context = boost::make_unique<Context>(
          *m_pimpl->m_eventsLog, *m_pimpl->m_tradingLog, std::move(settings));
    }
    const auto& settings = m_pimpl->m_context->GetSettings();
    const auto& config = settings.GetConfig();

    m_pimpl->m_context->SubscribeToStateUpdates(contextStateUpdateSlot);

    {
      const auto& tradingLogFilePath = settings.GetLogsDir() / "trading.log";
      if (config.get<bool>("general.tradingLog.isEnabled")) {
        m_pimpl->m_tradingLogFile.open(
            tradingLogFilePath.string().c_str(),
            std::ios::out | std::ios::ate | std::ios ::app);
        if (!m_pimpl->m_tradingLogFile) {
          boost::format error("Failed to open trading log file %1%.");
          error % tradingLogFilePath;
          throw Exception(error.str().c_str());
        }
        m_pimpl->m_eventsLog->Debug("Trading log: %1%.", tradingLogFilePath);
        m_pimpl->m_tradingLog->EnableStream(m_pimpl->m_tradingLogFile,
                                            *m_pimpl->m_context);
      } else {
        m_pimpl->m_eventsLog->Info("Trading log: DISABLED.");
      }
    }

    m_pimpl->m_context->Start(startProgressCallback, startErrorCallback,
                              dropCopy);
  } catch (const Exception& ex) {
    if (m_pimpl->m_eventsLog) {
      m_pimpl->m_eventsLog->Error("Failed to init engine context: \"%1%\".",
                                  ex.what());
    }
    boost::format message("Failed to init engine context: \"%1%\"");
    message % ex.what();
    throw Exception(message.str().c_str());
  }
}

trdk::Context& Engine::GetContext() {
  if (!m_pimpl->m_context) {
    throw Exception("Engine has no context as it not started");
  }
  return *m_pimpl->m_context;
}

void Engine::Stop(const trdk::StopMode& stopMode) {
  if (!m_pimpl->m_context) {
    throw Exception("Failed to stop engine, engine is stopped");
  }

  boost::thread stopThread([this, &stopMode]() {
    try {
      m_pimpl->m_context->Stop(stopMode);
    } catch (...) {
      AssertFailNoException();
      throw;
    }
  });
  stopThread.join();

  m_pimpl->m_context.reset();
}

void Engine::ClosePositions() {
  if (!m_pimpl->m_context) {
    throw Exception("Failed to close engine positions, engine is stopped");
  }
  m_pimpl->m_context->CloseSrategiesPositions();
}

void Engine::VerifyModules() const {
  try {
    Lib::VerifyModules([this](const fs::path& module) {
      m_pimpl->m_eventsLog->Debug("Found module %1%.", module);
    });
  } catch (const Exception& ex) {
    m_pimpl->m_eventsLog->Error("Failed to verify required modules: \"%1%\".",
                                ex.what());
    throw Exception("Failed to verify required modules");
  }
}
