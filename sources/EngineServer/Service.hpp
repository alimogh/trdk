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

namespace trdk { namespace EngineServer {

	class Client;

	class ServiceServer : private boost::noncopyable {

	public:

		ServiceServer();

	private:

		void StartAccept();
		void HandleNewClient(
				const boost::shared_ptr<Client> &,
				const boost::system::error_code &);

	private:

		boost::asio::io_service m_ioService;
		boost::asio::ip::tcp::acceptor m_acceptor;

		boost::thread m_thread;

	};

} }
