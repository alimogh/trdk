/**************************************************************************
 *   Created: 2015/03/17 01:13:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Server.hpp"

namespace trdk { namespace EngineServer {

	class Service : private boost::noncopyable {

	private:

		typedef autobahn::wamp_session<
				boost::asio::ip::tcp::socket,
				boost::asio::ip::tcp::socket>
			WampSession;

		struct Topics {

			std::string onNewInstance;
			std::string time;

			explicit Topics(const std::string &suffix);

		};

	public:

		explicit Service(
				const std::string &name,
				const boost::filesystem::path &);
		~Service();

	public:

		bool IsEngineStarted(const std::string &) const {
			//! @todo remove
			return false;
		}
		
	private:

		void Connect(const Lib::Ini &);
		void OnConnect(
				const std::string &host,
				uint16_t port,
				const boost::system::error_code &);
	
		void ScheduleNextCurrentTimeNotification();

		void PublishEngine();
		void PublishCurrentTime();

// 		void StartEngine(const std::string &commandInfo);
// 		void StopEngine(const std::string &engineId);
// 
// 		void OnContextStateChanges(
// 				trdk::Context &,
// 				const trdk::Context::State &,
// 				const std::string *message = nullptr);

	private:

		const std::string m_name;
		const std::string m_suffix;
		const Topics m_topics;

		std::ofstream m_logFile;
		EventsLog m_log;

		Settings m_settings;
		Server m_server;

		boost::asio::io_service m_io;
		boost::asio::ip::tcp::socket m_socket;
		std::shared_ptr<WampSession> m_session;
		boost::thread m_thread;

		boost::asio::deadline_timer m_currentTimePublishTimer;

		boost::future<void> m_joinFuture;
		boost::future<void> m_startFuture;

	};

} }
