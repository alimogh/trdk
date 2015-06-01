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
				EngineServer::Settings::EngineTransaction &,
				const std::string &commandInfo);
		virtual void StopEngine(const std::string &engineId);
		virtual Settings & GetEngineSettings(
				const std::string &engineId);

		virtual void UpdateStrategy(
				EngineServer::Settings::StrategyTransaction &);

		virtual FooSlotConnection Subscribe(
				const FooSlot &slot) {
			return m_fooSlotConnection.connect(slot);
		}
		

	private:

		void LoadEngine(const boost::filesystem::path &);

		void StartAccept();
		void HandleNewClient(
				const boost::shared_ptr<Client> &,
				const boost::system::error_code &);

		void CheckEngineIdExists(const std::string &id) const;

	private:

		const std::string m_name;

		std::map<
				std::string /* engine ID */,
				boost::shared_ptr<Settings>>
			m_engines;

		Server m_server;

		boost::asio::io_service m_ioService;
		boost::asio::ip::tcp::acceptor m_acceptor;

		boost::thread m_thread;

		boost::signals2::signal<FooSlotSignature> m_fooSlotConnection;

	};

} }
