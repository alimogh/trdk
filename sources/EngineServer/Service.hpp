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

#ifndef TRDK_AUTOBAHN_DISABLED

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

		class Exception : public EngineServer::Exception {
		public:
			explicit Exception(const char *what) throw()
				: EngineServer::Exception(what) {
				//...//
			}
		};
		class ConnectError : public Exception {
		public:
			explicit ConnectError(const char *what) throw()
				: Exception(what) {
				//...//
			}
		};
		class TimeoutException : public Exception {
		public:
			explicit TimeoutException(const char *what) throw()
				: Exception(what) {
				//...//
			}
		};

		//! @todo Legacy support, to remove
		struct InputIo {
		
			boost::asio::io_service service;
			boost::asio::ip::tcp::acceptor acceptor;

			InputIo();
		
		};

		struct Config {

			std::string name;
			std::string id;
			std::string twsHost;
			uint16_t twsPort;
			bool wampDebug;
			boost::posix_time::time_duration timeout;

			explicit Config(const boost::filesystem::path &);

		};

		struct Topics {

			std::string time;
			std::string state;

			explicit Topics(const std::string &suffix);

		};

		typedef boost::mutex ConnectionMutex;
		typedef boost::unique_lock<ConnectionMutex> ConnectionLock;
		typedef boost::condition_variable ConnectionCondition;

		class Connection
			: private boost::noncopyable
			, public boost::enable_shared_from_this<Connection> {

		public:

			typedef autobahn::wamp_session<
					boost::asio::ip::tcp::socket,
					boost::asio::ip::tcp::socket>
				Session;

			EventsLog &log;
			const Topics &topics;
			boost::asio::ip::tcp::socket socket;
			boost::asio::deadline_timer currentTimePublishTimer;
			boost::asio::deadline_timer ioTimeoutTimer;
			std::shared_ptr<Session> session;

			boost::shared_future<void> sessionStartFuture;
			boost::shared_future<void> sessionJoinFuture;
			boost::shared_future<void> engineRegistrationFuture;

			explicit  Connection(
					boost::asio::io_service &io,
					const Topics &topics,
					EventsLog &log,
					bool isWampDebugOn);

			void ScheduleNextCurrentTimeNotification();
			void ScheduleIoTimeout(const boost::posix_time::time_duration &);
			void StopIoTimeout();

		};

	public:

		explicit Service(
				const std::string &name,
				const boost::filesystem::path &);
		//! @todo Virtual - is legacy support, remove virtual
		virtual ~Service();

	public:

		bool IsEngineStarted() const;

	public:

		//! @todo Legacy support, to remove
		virtual const std::string & GetName() const {
			return m_config.name;
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
		void StartAccept();
		//! @todo Legacy support, to remove
		void HandleNewClient(
				const boost::shared_ptr<Client> &,
				const boost::system::error_code &);

		//! @todo Legacy support, to remove
		void CheckEngineIdExists(const std::string &id) const;

		void OnContextStateChanged(
				trdk::Context &,
				const trdk::Context::State &,
				const std::string *message = nullptr);

	private:

		void StartIoThread();

		boost::shared_ptr<Connection> Connect();
		void OnConnected(
			boost::shared_ptr<Connection>,
			const boost::system::error_code &);
		void OnSessionStarted(boost::shared_ptr<Connection>, bool isStarted);
		void OnSessionJoined(
			boost::shared_ptr<Connection>,
			const boost::optional<uint64_t> &sessionId);
		void OnEngineRegistered(
			boost::shared_ptr<Connection>,
			uint64_t sessionId,
			const boost::optional<uint64_t> &instanceId,
			const std::tuple<std::string, std::string, std::string> &);

		void Reconnect();
		void RepeatReconnection(const Exception &prevReconnectError);

		void PublishState() const;

	private:

		const Config m_config;

        //! Log file.
        /** Should be first to be removed last.
          */
        std::ofstream m_logFile;
        EventsLog m_log;

		Settings m_settings;
		Server m_server;

		const Topics m_topics;

		//! @todo Legacy support, to remove
		std::unique_ptr<InputIo> m_inputIo;
		//! @todo Legacy support, to remove
		boost::thread m_inputIoThread;
		//! @todo Legacy support, to remove
		ConnectionsMutex m_connectionsMutex;
		//! @todo Legacy support, to remove
		std::set<Client *> m_connections;

		size_t m_numberOfReconnects;

		boost::thread m_thread;
		//! IO service.
		/** Should be last, to be removed first - all objects in service should
		  * be removed before other objects in "this".
		  */
		std::unique_ptr<boost::asio::io_service> m_io;

		mutable ConnectionMutex m_connectionMutex;
		ConnectionCondition m_connectionCondition;
		boost::shared_ptr<Connection> m_connection;
		bool m_isInited;

    };

} }

#endif
