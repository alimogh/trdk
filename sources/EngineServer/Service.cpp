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

EngineServer::Service::Io::Io()
	:	acceptor(
			service,
			io::ip::tcp::endpoint(io::ip::tcp::v4(), 3689)) {
	//...//
}

EngineServer::Service::Service(
		const std::string &name,
		const fs::path &engineConfigFilePath)
	: m_name(name),
	m_io(new Io) {

	LoadEngine(engineConfigFilePath);

	{
		const auto &endpoint = m_io->acceptor.local_endpoint();
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
				m_io->service.run();
				std::cout << "IO task completed." << std::endl;
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		});

}

EngineServer::Service::~Service() {
	try {
		m_io->service.stop();
		m_thread.join();
	} catch (...) {
		AssertFailNoException();
		throw;
	}
	m_io.reset();
}

void EngineServer::Service::LoadEngine(const fs::path &configFilePath) {

	boost::shared_ptr<Settings> settings(
		new Settings(configFilePath, GetName(), *this));
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

void EngineServer::Service::UpdateEngine(
		EngineServer::Settings::EngineTransaction &transaction) {
	m_server.Update(transaction);
}

void EngineServer::Service::UpdateStrategy(
		EngineServer::Settings::StrategyTransaction &transaction) {
	m_server.Update(transaction);
}

void EngineServer::Service::StartEngine(
		const std::string &engineId,
		const std::string &commandInfo) {
	const auto &settings = GetEngineSettings(engineId);
	Context &context = m_server.Run(
		m_fooSlotConnection,
		settings.GetEngeineId(),
		settings.GetFilePath(),
		false,
		commandInfo);
	context.SubscribeToStateUpdate(
		boost::bind(
			&Service::OnContextStateChanges,
			this,
			boost::ref(context),
			_1,
			_2));
	context.RaiseStateUpdate(Context::STATE_STARTED);
}

void EngineServer::Service::StopEngine(const std::string &engineId) {
	CheckEngineIdExists(engineId);
	m_server.StopAll(STOP_MODE_GRACEFULLY_ORDERS);
}

void EngineServer::Service::StartAccept() {
	const auto &newConnection = Client::Create(
		m_io->acceptor.get_io_service(),
		*this);
    m_io->acceptor.async_accept(
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
		const ConnectionsWriteLock lock(m_connectionsMutex);
		auto list(m_connections);
		list.insert(&*newConnection);
		newConnection->Start();
		list.swap(m_connections);
	} else {
		//! @todo Write to log
		std::cerr
			<< "Failed to accept new client: \""
			<< SysError(error.value()) << "\"."
			<< std::endl;
	}
	StartAccept();
}

void EngineServer::Service::OnDisconnect(Client &client) {
	const ConnectionsWriteLock lock(m_connectionsMutex);
	m_connections.erase(&client);
}

void EngineServer::Service::CheckEngineIdExists(const std::string &id) const {
	if (m_engines.find(id) == m_engines.end()) {
		boost::format message("Engine with ID \"%1%\" doesn't exist.");
		message % id;
		throw EngineServer::Exception(message.str().c_str());
	}
}

void EngineServer::Service::OnContextStateChanges(
		Context &,
		const Context::State &state,
		const std::string *updateMessage /*= nullptr*/) {
	boost::function<void(Client &)> fun;
	static_assert(Context::numberOfStates == 3, "List changed.");
	std::string message;
	switch (state) {
		case Context::STATE_STARTED:
			message = updateMessage
				?	(boost::format("Engine started: %1%.") % *updateMessage).str()
				:	"Engine started.";
			fun = [&message](Client &client) {
				client.SendMessage(message);
			};
			break;
		case Context::STATE_STOPPED_GRACEFULLY:
			message = updateMessage
				?	(boost::format("Engine stopped: %1%.") % *updateMessage).str()
				:	"Engine stopped.";
			fun = [&message](Client &client) {
				client.SendMessage(message);
			};
			break;
		case Context::STATE_STOPPED_ERROR:
		default:
			message = updateMessage
				?	(boost::format("Engine stopped with error: %1%.") % *updateMessage).str()
				:	"Engine stopped with error.";
			fun = [&message](Client &client) {
				client.SendError(message);
			};
			break;

	}
	const ConnectionsReadLock lock(m_connectionsMutex);
	foreach (Client *client, m_connections) {
		fun(*client);
	}
}
