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
using namespace trdk::Lib;
using namespace trdk::EngineServer;

namespace io = boost::asio;
namespace fs = boost::filesystem;

EngineServer::Service::Service(
		const std::string &name,
		const fs::path &engineConfigFilePath)
	: m_name(name),
	m_acceptor(
		m_ioService,
		io::ip::tcp::endpoint(io::ip::tcp::v4(), 3689)) {

	LoadEngine(engineConfigFilePath);

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

void EngineServer::Service::LoadEngine(const fs::path &configFilePath) {

	boost::shared_ptr<Settings> settings(
		new Settings(configFilePath, GetName()));
	if (m_engines.find(settings->GetEngeineId()) != m_engines.end()) {
		boost::format message("Engine with ID \"%1%\" already loaded");
		message % settings->GetEngeineId();
		throw EngineServer::Exception(message.str().c_str());
	}

	m_engines[settings->GetEngeineId()] = settings;
	//! @todo Write to log
	std::cout
		<< "Loaded engine \"" << settings->GetEngeineId() << "\""
		<< " from " << configFilePath << "." << std::endl;

}

void EngineServer::Service::ForEachEngineId(
		const boost::function<void(const std::string &engineId)> &func)
		const {
	foreach (const auto &engine, m_engines) {
		func(engine.first);
	}
}

bool EngineServer::Service::IsEngineStarted(const std::string &engineId) const {
	CheckEngineIdExists(engineId);
	return m_server.IsStarted(engineId);
}

EngineServer::Settings & EngineServer::Service::GetEngineSettings(
		const std::string &engineId) {
	CheckEngineIdExists(engineId);
	// return m_engines[engineId];
	return *m_engines.find(engineId)->second;
}

void EngineServer::Service::UpdateStrategy(
		EngineServer::Settings::StrategyTransaction &/*transaction*/) {
	//...//				
}

void EngineServer::Service::StartEngine(
		EngineServer::Settings::EngineTransaction &transaction,
		const std::string &commandInfo) {
	m_server.Run(
		m_fooSlotConnection,
		transaction,
		false,
		commandInfo);
}

void EngineServer::Service::StopEngine(const std::string &engineId) {
	CheckEngineIdExists(engineId);
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
			<< "Failed to accept new client: \""
			<< SysError(error.value()) << "\"."
			<< std::endl;
	}
	StartAccept();
}

void EngineServer::Service::CheckEngineIdExists(const std::string &id) const {
	if (m_engines.find(id) == m_engines.end()) {
		boost::format message("Engine with ID \"%1%\" doesn't exist.");
		message % id;
		throw EngineServer::Exception(message.str().c_str());
	}
}
