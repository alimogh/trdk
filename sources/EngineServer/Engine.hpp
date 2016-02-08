/**************************************************************************
 *   Created: 2013/02/03 11:26:52
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Engine/Context.hpp"
#include "Core/TradingLog.hpp"

namespace trdk { namespace EngineServer {

	class Engine : private boost::noncopyable {

	public:

		explicit Engine(
				const boost::filesystem::path &,
				const trdk::Engine::Context::StateUpdateSlot &contextStateUpdateSlot,
				bool enableStdOutLog);
		explicit Engine(
				const boost::filesystem::path &,
				const trdk::Engine::Context::StateUpdateSlot &contextStateUpdateSlot,
				DropCopy &dropCopy,
				bool enableStdOutLog);
		~Engine();

	public:

		void Stop(const trdk::StopMode &);

		void ClosePositions();

	private:

		void Run(
				const boost::filesystem::path &,
				const trdk::Context::StateUpdateSlot &,
				DropCopy *dropCopy,
				bool enableStdOutLog);

		void VerifyModules() const;

	private:

		std::ofstream m_eventsLogFile;
		mutable trdk::Engine::Context::Log m_eventsLog;

		std::ofstream m_tradingLogFile;
		trdk::Engine::Context::TradingLog m_tradingLog;

		std::unique_ptr<trdk::Engine::Context> m_context;

	};

} }
