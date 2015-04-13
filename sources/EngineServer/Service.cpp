/**************************************************************************
 *   Created: 2015/03/17 01:13:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "Service.hpp"
#include "Client.hpp"
#include "Exception.hpp"

using namespace trdk;
using namespace trdk::EngineServer;

namespace io = boost::asio;
namespace fs = boost::filesystem;

EngineServer::Service::Service(const fs::path &engineConfigFilePath)
	: m_acceptor(
		m_ioService,
		io::ip::tcp::endpoint(io::ip::tcp::v4(), 3689)) {

	LoadEngine("service", engineConfigFilePath);

	{
		const auto &endpoint = m_acceptor.local_endpoint();
		//! @todo Write to log
		std::cout
			<< "Opening endpoint "
			<< endpoint.address() << ":" << endpoint.port()
			<< " for incoming connections..." << std::endl;
	}

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

EngineServer::Service::~Service() {
	try {
		m_ioService.stop();
		m_thread.join();
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

void EngineServer::Service::LoadEngine(
		const std::string &engineId,
		const fs::path &configFilePath) {

	//! @todo Write to log
	std::cout << "Loading engine \"" << engineId << "\"..." << std::endl;

	if (IsEngineExists(engineId)) {
		boost::format message("Engine with ID \"%1%\" already loaded");
		message % engineId;
		throw EngineServer::Exception(message.str().c_str());
	}

	m_engines[engineId] = configFilePath;

}

void EngineServer::Service::ForEachEngineId(
		const boost::function<void (const std::string &engineId)> &func)
		const {
	foreach (const auto &engine, m_engines) {
		func(engine.first);
	}
}

bool EngineServer::Service::IsEngineExists(const std::string &engineId) const {
	return m_engines.find(engineId) != m_engines.end();
}

bool EngineServer::Service::IsEngineStarted(const std::string &engineId) const {
	if (!IsEngineExists(engineId)) {
		//! @todo Write to log
		std::cerr
			<< "Failed to check engine state: Engine with ID \""
			<< engineId << "\" doesn't exist." << std::endl;
		return false;
	}
	return m_server.IsStarted(engineId);
}

const fs::path & EngineServer::Service::GetEngineSettings(
		const std::string &engineId)
		const {
	if (!IsEngineExists(engineId)) {
		boost::format message("Engine with ID \"%1%\" does not exist");
		message % engineId;
		throw EngineServer::Exception(message.str().c_str());
	}
	// return m_engines[engineId];
	return m_engines.find(engineId)->second;
}

void EngineServer::Service::StartEngine(
		const std::string &engineId,
		Client &client) {

	//! @todo Write to log
	std::cout
		<< "Starting engine \"" << engineId << "\" by request from "
		<< client.GetRemoteAddressAsString() << "..." << std::endl;

	const auto &engine = m_engines.find(engineId);
	if (engine == m_engines.end()) {
		Assert(!IsEngineExists(engineId));
		//! @todo Write to log
		std::cerr
			<< "Failed to start engine: Engine with ID \""
			<< engineId << "\" doesn't exist." << std::endl;
		return;
	}
	Assert(IsEngineExists(engineId));

	boost::format commandInfo("%1% %2%");
	commandInfo % client.GetRemoteAddressAsString() % engine->second;

	try {
		m_server.Run(engine->first, engine->second, false, commandInfo.str());
	} catch (const EngineServer::Exception &ex) {
		//! @todo Write to log
		std::cerr
			<< "Failed to start engine: " << ex.what() << "." << std::endl;
	}

}

void EngineServer::Service::StopEngine(
		const std::string &engineId,
		Client &client) {

	//! @todo Write to log
	std::cout
		<< "Stopping engine \"" << engineId << "\" by request from "
		<< client.GetRemoteAddressAsString() << "..." << std::endl;


	if (!IsEngineExists(engineId)) {
		//! @todo Write to log
		std::cerr
			<< "Failed to stop engine: Engine with ID \""
			<< engineId << "\" doesn't exist." << std::endl;
		return;
	}

	m_server.StopAll(STOP_MODE_GRACEFULLY_ORDERS);

}

void EngineServer::Service::StartAccept() {
	const auto &newConnection = Client::Create(
		m_acceptor.get_io_service(),
		*this);
    m_acceptor.async_accept(
		newConnection->GetSocket(),
		boost::bind(
			&Service::HandleNewClient,
			this,
			newConnection,
			io::placeholders::error));
}

void EngineServer::Service::HandleNewClient(
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
