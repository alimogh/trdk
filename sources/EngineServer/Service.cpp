

#include "Prec.hpp"
#include "Service.hpp"
#include "Client.hpp"

using namespace trdk;
using namespace trdk::EngineServer;

namespace io = boost::asio;

ServiceServer::ServiceServer()
		: m_acceptor(
			m_ioService,
			io::ip::tcp::endpoint(io::ip::tcp::v4(), 3689)) {

	//! @todo Write to log
	std::cout << "Opening port 3689 for incoming connections ..." << std::endl;

	StartAccept();

	m_thread = boost::thread(
		[&]() {
			try {
				//! @todo Write to log
				std::cout << "Starting IO task..." << std::endl;
				//! @todo Write to log
				m_ioService.run();
				std::cout << "IO task completed." << std::endl;
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		});

}

void ServiceServer::StartAccept() {
	const auto &newConnection = Client::Create(m_acceptor.get_io_service());
    m_acceptor.async_accept(
		newConnection->GetSocket(),
		boost::bind(
			&ServiceServer::HandleNewClient,
			this,
			newConnection,
			io::placeholders::error));
}

void ServiceServer::HandleNewClient(
		const boost::shared_ptr<Client> &newConnection,
		const boost::system::error_code &error) {
	if (!error) {
		newConnection->Start();
	} else {
		//! @todo Write to log
		std::cerr
			<< "Failed to accept new client: \"" << error << "\"."
			<< std::endl;
	}
	StartAccept();
}
