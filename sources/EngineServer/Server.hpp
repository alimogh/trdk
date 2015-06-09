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

#include "Settings.hpp"
#include "Engine/Context.hpp"
#include "Core/TradingLog.hpp"

namespace trdk { namespace EngineServer {

	class Server : private boost::noncopyable {

	private:

		typedef boost::mutex Mutex;
		typedef Mutex::scoped_lock Lock;

		struct EngineInfo {

			std::string id;

			boost::shared_ptr<std::ofstream> eventsLogFile;
			boost::shared_ptr<Engine::Context::Log> eventsLog;
			boost::shared_ptr<std::ofstream> tradingLogFile;
			boost::shared_ptr<Engine::Context::TradingLog> tradingLog;

			boost::shared_ptr<trdk::Engine::Context> engine;

		};

		struct ById {
			//...//
		};

		typedef boost::multi_index_container<
				EngineInfo,
				boost::multi_index::indexed_by<
					boost::multi_index::hashed_unique<
						boost::multi_index::tag<ById>,
						boost::multi_index::member<
							EngineInfo,
							std::string,
							&EngineInfo::id>>>>
			Engines;

	public:

		Server();

	public:

		bool IsStarted(const std::string &id) const;

		trdk::Context & Run(
				boost::signals2::signal<FooSlotSignature> &fooSlotConnection,
				const std::string &id,
				const boost::filesystem::path &,
				bool enableStdOutLog,
				const std::string &commandInfo);
		void StopAll(const trdk::StopMode &);

		void Update(EngineServer::Settings::Transaction &);

	private:

		mutable Mutex m_mutex;
		Engines m_engines;

	};

} }
