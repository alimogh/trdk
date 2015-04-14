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

	public:

		explicit Service(const boost::filesystem::path &);
		virtual ~Service();
	
	public:

		bool IsEngineExists(const std::string &engineId) const;

	public:

		virtual void ForEachEngineId(
				const boost::function<void (const std::string &engineId)> &)
				const;
		virtual bool IsEngineStarted(const std::string &engineId) const;
		virtual void StartEngine(const std::string &engineId, Client &);
		virtual void StopEngine(const std::string &engineId, Client &);
		virtual const boost::filesystem::path & GetEngineSettings(
				const std::string &engineId)
				const;

		virtual FooSlotConnection Subscribe(
				const FooSlot &slot) {
			return m_fooSlotConnection.connect(slot);
		}
		

	private:

		void LoadEngine(
				const std::string &engineId,
				const boost::filesystem::path &);

		void StartAccept();
		void HandleNewClient(
				const boost::shared_ptr<Client> &,
				const boost::system::error_code &);

	private:

		std::map<
				std::string /* engine ID */,
				boost::filesystem::path /* path to settings file */>
			m_engines;

		Server m_server;

		boost::asio::io_service m_ioService;
		boost::asio::ip::tcp::acceptor m_acceptor;

		boost::thread m_thread;

		boost::signals2::signal<FooSlotSignature> m_fooSlotConnection;

	};

} }
