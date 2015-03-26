/**************************************************************************
 *   Created: 2015/03/22 21:10:24
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Client.hpp"

class Server : private boost::noncopyable, public Client::Handler {

private:

	struct ClientInfo {
		size_t subscribedPairsCount;
		bool isActive;
		boost::shared_ptr<std::ifstream> f;
	};

public:

	Server();
	virtual ~Server() {
		//...//
	}

public:

	virtual void OnConnectionClosed(Client &);

	virtual void OnDataSent(Client &, const char *data, size_t size);
	virtual void OnNewData(Client &, const char *data, size_t size);

private:

	void StartAccept();
	void HandleNewItchClient(
			const boost::shared_ptr<Client> &,
			const boost::system::error_code &);
	void HandleNewControlClient(
			const boost::shared_ptr<Client> &,
			const boost::system::error_code &);
	void SendNextPacket(Client &);

private:

	boost::asio::io_service m_itchIoService;
	boost::asio::ip::tcp::acceptor m_itchAcceptor;

	boost::asio::io_service m_controlIoService;
	boost::asio::ip::tcp::acceptor m_controlAcceptor;

	boost::thread_group m_itchThread;
	boost::thread_group m_controlThread;

	std::map<boost::shared_ptr<Client>, ClientInfo> m_itchClients;
	std::set<boost::shared_ptr<Client>> m_controlClients;

	uint32_t m_next;
	std::map<uint32_t, boost::chrono::high_resolution_clock::time_point> m_packets;
	Concurrency::critical_section m_packetsLock;

	intmax_t m_sum;
	intmax_t m_count;
	intmax_t m_min;
	intmax_t m_max;

};
