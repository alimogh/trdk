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

		typedef boost::shared_mutex ConnectionsMutex;
		typedef boost::shared_lock<ConnectionsMutex> ConnectionsReadLock;
		typedef boost::unique_lock<ConnectionsMutex> ConnectionsWriteLock;

	private:

		struct Io {
		
			boost::asio::io_service service;
			boost::asio::ip::tcp::acceptor acceptor;

			Io();
		
		};

	public:

		explicit Service(
				const std::string &name,
				const boost::filesystem::path &);
		virtual ~Service();

	public:

		virtual const std::string & GetName() const {
			return m_name;
		}

		virtual void ForEachEngineId(
				const boost::function<void(const std::string &engineId)> &)
				const;
		virtual bool IsEngineStarted(const std::string &engineId) const;
		virtual void StartEngine(
				const std::string &engineId,
				const std::string &commandInfo);
		virtual void StopEngine(const std::string &engineId);
		virtual Settings & GetEngineSettings(
				const std::string &engineId);

		virtual void ClosePositions(const std::string &engineId);

		virtual void UpdateEngine(
				EngineServer::Settings::EngineTransaction &);
		virtual void UpdateStrategy(
				EngineServer::Settings::StrategyTransaction &);

		virtual void OnDisconnect(Client &);
	
	private:

		void LoadEngine(const boost::filesystem::path &);

		void StartAccept();
		void HandleNewClient(
				const boost::shared_ptr<Client> &,
				const boost::system::error_code &);

		void CheckEngineIdExists(const std::string &id) const;

		void OnContextStateChanges(
				trdk::Context &,
				const trdk::Context::State &,
				const std::string *message = nullptr);

	private:

		const std::string m_name;

		std::map<
				std::string /* engine ID */,
				boost::shared_ptr<Settings>>
			m_engines;

		Server m_server;

		std::unique_ptr<Io> m_io;

		boost::thread m_thread;

		ConnectionsMutex m_connectionsMutex;
		std::set<Client *> m_connections;

	};

} }
