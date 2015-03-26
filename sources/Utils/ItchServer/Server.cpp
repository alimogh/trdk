/**************************************************************************
 *   Created: 2015/03/22 21:30:47
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Server.hpp"

namespace io = boost::asio;
namespace pt = boost::posix_time;

Server::Server()
	: m_itchAcceptor(
		m_itchIoService,
		io::ip::tcp::endpoint(io::ip::tcp::v4(), 9013)),
	m_controlAcceptor(
		m_controlIoService,
		io::ip::tcp::endpoint(io::ip::tcp::v4(), 9014)),
	m_next(1234567),
	m_sum(0),
	m_count(0),
	m_min(0),
	m_max(0) {

	std::cout
		<< "Opening ports 9013 and 9014 for incoming connections ..."
		<< std::endl;

	StartAccept();

	for (size_t i = 0; i < 10; ++i) {
		m_itchThread.create_thread(
			[&]() {
				try {
					std::cout << "Starting ITCH IO task..." << std::endl;
					m_itchIoService.run();
					std::cout << "ITCH IO task completed." << std::endl;
				} catch (...) {
					assert(false);
					throw;
				}
			});
	}
	for (size_t i = 0; i < 10; ++i) {
		m_controlThread.create_thread(
			[&]() {
				try {
					std::cout << "Starting Control IO task..." << std::endl;
					m_controlIoService.run();
					std::cout << "Control IO task completed." << std::endl;
				} catch (...) {
					assert(false);
					throw;
				}
			});
	}

}

void Server::StartAccept() {
	{
		const auto &newConnection
			= Client::Create(m_itchAcceptor.get_io_service(), *this, 1);
		m_itchAcceptor.async_accept(
			newConnection->GetSocket(),
			boost::bind(
				&Server::HandleNewItchClient,
				this,
				newConnection,
				io::placeholders::error));
	}
	{
		const auto &newConnection
			= Client::Create(m_controlAcceptor.get_io_service(), *this, -1);
		m_controlAcceptor.async_accept(
			newConnection->GetSocket(),
			boost::bind(
				&Server::HandleNewControlClient,
				this,
				newConnection,
				io::placeholders::error));
	}
}

void Server::HandleNewItchClient(
		const boost::shared_ptr<Client> &newConnection,
		const boost::system::error_code &error) {
	std::cout << "Accepting new ITCH client..." << std::endl;
	if (!error) {
		ClientInfo info = {};
		m_itchClients[newConnection] = info;
		newConnection->Start();
		std::cout << "Accepted new ITCH client." << std::endl;
	} else {
		std::cerr
			<< "Failed to accept new ITCH client: \"" << error << "\"."
			<< std::endl;
	}
}

void Server::HandleNewControlClient(
		const boost::shared_ptr<Client> &newConnection,
		const boost::system::error_code &error) {
	std::cout << "Accepting new ITCH client..." << std::endl;
	if (!error) {
		m_controlClients.insert(newConnection);
		newConnection->Start();
		std::cout << "Accepted new Control client." << std::endl;
	} else {
		std::cerr
			<< "Failed to accept new Control client: \"" << error << "\"."
			<< std::endl;
	}
}

void Server::OnConnectionClosed(Client &client) {
	if (client.GetFlag() > 0) {
		assert(m_itchClients.find(client.shared_from_this()) != m_itchClients.end());
		m_itchClients.erase(client.shared_from_this());
	} else {
		assert(m_controlClients.find(client.shared_from_this()) != m_controlClients.end());
		m_controlClients.erase(client.shared_from_this());
	}
}

void Server::OnDataSent(Client &client, const char *, size_t /*size*/) {
	if (client.GetFlag() > 0) {
		if (m_itchClients[client.shared_from_this()].isActive) {
			// boost::this_thread::sleep(pt::microseconds(1));
			SendNextPacket(client);
		}
	} else {
		assert(false);
	}
}

void Server::OnNewData(Client &client, const char *data, size_t size) {
	const auto now = boost::chrono::high_resolution_clock::now();
	if (client.GetFlag() > 0) {
		switch (data[0]) {
			case 'L':
				client.Send("A         1");
				break;
			case 'R':
				break;
			case 'M':
				std::cout << "Snapshot requested." << std::endl;
				break;
			case 'A':
				std::cout << "Subscription requested." << std::endl;
				{
					auto &info = m_itchClients[client.shared_from_this()];
					if (++info.subscribedPairsCount >= 3) {
						if (!info.isActive) {
							std::cout << "Starting online data..." << std::endl;
							info.isActive = true;
							info.f.reset(
								new std::ifstream(
									"a:/Projects/trdk/sources/Utils/ItchServer/Debug/itch.dump",
									std::ios::binary));
							assert(*info.f);
							SendNextPacket(client);
						}
					}
				}
				break;
			default:
				std::cout << "Unknown request \"" << data[0] << "\"." << std::endl;
				client.GetSocket().close();
				break;
		}
	} else {
		if (size % sizeof(uint32_t)) {
			std::cout << "Proto error 1! " << size << std::endl;
			_getch();
		}
		for (size_t i = 0; size > 0; size -= sizeof(uint32_t), ++i) {
			const uint32_t packet
				= *(reinterpret_cast<const uint32_t *>(data) + i);
			boost::chrono::high_resolution_clock::time_point p;
			{
				const Concurrency::critical_section::scoped_lock lock(m_packetsLock);
				auto it = m_packets.find(packet);
				if (it == m_packets.end()) {
					std::cout << "Proto error 2! " << packet << std::endl;
					//_getch();
					continue;
				}
				p = it->second;
				m_packets.erase(it);
			}
			const auto period = now - p;
			const auto diff = boost::chrono::duration_cast<boost::chrono::microseconds>(period).count();
			if (!m_min || diff < m_min) {
				m_min = diff;
			}
			if (diff > m_max)  {
				m_max = diff;
			}
			m_sum += diff;
			if (!(++m_count % 1000)) {
 				std::cout << m_count << ":\t"
 					 << (m_sum / m_count) << "\t"
 					<< m_min << "\t"
 					<< m_max << std::endl;
				m_count
					= m_max
					= m_min
					= m_sum
					= 0;
			}
		}
	}
}

void Server::SendNextPacket(Client &client) {
	ClientInfo &info = m_itchClients[client.shared_from_this()];
	assert(*info.f);
	char ch = 0;
	info.f->read(&ch, sizeof(ch));
	if (ch != '|') {
		std::cerr << "Storage has wrong format." << std::endl;
		return;
	}
	uint32_t size = 0;
	info.f->read(reinterpret_cast<char *>(&size), sizeof(size));

	boost::shared_ptr<std::vector<char>> data(new std::vector<char>(size));
	info.f->read(data->data(), size);

	bool isS = false;
	const size_t start = m_next;
	for (std::vector<char>::iterator i = data->begin(); i != data->end(); ++i) {
		if (*i == 0x0A) {
			isS = false;
		} else if (!isS) {
			if (*i == 'S') {
				isS = true;
			}
		} else if (*i == 'N' || *i == 'M') {
			if ((data->end() - i) <= (*i == 'N' ? 34 : 23) + 16) {
				break;
			}
			std::string s = boost::lexical_cast<std::string>(m_next++);
			s.insert(s.end(), 16 - s.size(), ' ');
			std::copy(s.begin(), s.end(), i + (*i == 'N' ? 34 : 23));
			isS = false;
		}
	}

	{
		const auto &now =  boost::chrono::high_resolution_clock::now();
		const Concurrency::critical_section::scoped_lock lock(m_packetsLock);
		for (size_t i = start; i < m_next; ++i) {
			m_packets[i] = now;
		}
	}

	client.Send(data);

}
