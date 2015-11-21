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
#include "ClientRequestHandler.hpp"

namespace trdk { namespace EngineServer {

	class Client;

	class Service : public ClientRequestHandler {

	private:

		//! @todo Legacy support, to remove
		typedef boost::shared_mutex ConnectionsMutex;
		//! @todo Legacy support, to remove
		typedef boost::shared_lock<ConnectionsMutex> ConnectionsReadLock;
		//! @todo Legacy support, to remove
		typedef boost::unique_lock<ConnectionsMutex> ConnectionsWriteLock;

	private:

		//! @todo Legacy support, to remove
		struct InputIo {
		
			boost::asio::io_service service;
			boost::asio::ip::tcp::acceptor acceptor;

			InputIo();
		
		};

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
		//! @todo Virtual - is alegacy suppor, remove virtual
		virtual ~Service();

	public:

		//! @todo Legacy support, to remove
		virtual const std::string & GetName() const {
			return m_name;
		}

		//! @todo Legacy support, to remove
		virtual void ForEachEngineId(
				const boost::function<void(const std::string &engineId)> &)
				const;
		//! @todo Virtual - is alegacy suppor, remove virtual
		virtual bool IsEngineStarted(const std::string &engineId) const;
		//! @todo Legacy support, to remove
		virtual void StartEngine(
				const std::string &engineId,
				const std::string &commandInfo);
		//! @todo Legacy support, to remove
		virtual void StopEngine(const std::string &engineId);
		//! @todo Legacy support, to remove
		virtual Settings & GetEngineSettings(
				const std::string &engineId);

		//! @todo Legacy support, to remove
		virtual void ClosePositions(const std::string &engineId);

		//! @todo Legacy support, to remove
		virtual void UpdateEngine(
				EngineServer::Settings::EngineTransaction &);
		//! @todo Legacy support, to remove
		virtual void UpdateStrategy(
				EngineServer::Settings::StrategyTransaction &);

		//! @todo Legacy support, to remove
		virtual void OnDisconnect(Client &);
	
	private:

		//! @todo Legacy support, to remove
		void LoadEngine(const boost::filesystem::path &);

		//! @todo Legacy support, to remove
		void StartAccept();
		//! @todo Legacy support, to remove
		void HandleNewClient(
				const boost::shared_ptr<Client> &,
				const boost::system::error_code &);

		//! @todo Legacy support, to remove
		void CheckEngineIdExists(const std::string &id) const;

		void OnContextStateChanges(
				trdk::Context &,
				const trdk::Context::State &,
				const std::string *message = nullptr);

	private:

		void Connect(const Lib::Ini &);
		void OnConnect(
				const std::string &host,
				uint16_t port,
				const boost::system::error_code &);
	
		void ScheduleNextCurrentTimeNotification();

		void PublishEngine();
		void PublishCurrentTime();

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

		//! @todo Legacy support, to remove
		std::map<
				std::string /* engine ID */,
				boost::shared_ptr<Settings>>
			m_engines;

		//! @todo Legacy support, to remove
		std::unique_ptr<InputIo> m_inputIo;
		//! @todo Legacy support, to remove
		boost::thread m_inputIoThread;
		//! @todo Legacy support, to remove
		ConnectionsMutex m_connectionsMutex;
		//! @todo Legacy support, to remove
		std::set<Client *> m_connections;

	};

} }
