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
namespace pt = boost::posix_time;

EngineServer::Service::Topics::Topics(const std::string &suffix)
	: onNewInstance("trdk.engine.on_new_instance")
	, time((boost::format("trdk.engine.%1%.time") % suffix).str()) {
	//...//
}

EngineServer::Service::InputIo::InputIo()
	:	acceptor(
			service,
			io::ip::tcp::endpoint(io::ip::tcp::v4(), 3689)) {
	//...//
}

EngineServer::Service::Service(
		const std::string &name,
		const fs::path &engineConfigFilePath)
	: m_name(name)
	, m_suffix(boost::replace_all_copy(boost::to_lower_copy(m_name), ".", "_"))
	, m_topics(m_suffix)
	, m_settings(engineConfigFilePath, m_name, *this)
	, m_socket(m_io)
	, m_currentTimePublishTimer(m_io)
	, m_inputIo(new InputIo) {

	m_log.EnableStdOut();

	const IniFile settings(m_settings.GetFilePath());

	const auto logFilePath = settings.ReadFileSystemPath("General", "logs_dir") / "service.log";
	fs::create_directories(logFilePath.branch_path());
	m_logFile.open(logFilePath.string().c_str(), std::ios::out | std::ios::ate | std::ios::app);
	if (!m_logFile) {
		m_log.Error("Failed to open file %1% for logging.", logFilePath);
		throw EngineServer::Exception("Failed to open file for logging");
	}
	m_log.EnableStream(m_logFile, true);

	m_session = std::make_shared<WampSession>(
		m_io,
		m_socket,
		m_socket,
		settings.ReadBoolKey("Service", "wamp_debug", false));

	m_log.Info(
		"Loaded engine \"%1%\" from %2% for engine service \"%3%\".",
		m_settings.GetEngineId(),
		m_settings.GetFilePath(),
		m_name);

	LoadEngine(engineConfigFilePath);

	{
		const auto &endpoint = m_inputIo->acceptor.local_endpoint();
		//! @todo Write to log
		std::cout
			<< "Opening endpoint "
			<< endpoint.address() << ":" << endpoint.port()
			<< " for incoming connections..." << std::endl;
	}

	StartAccept();
	Connect(settings);

	m_inputIoThread = boost::thread(
		[&]() {
			try {
				//! @todo Write to log
				std::cout << "Starting IO task..." << std::endl;
				//! @todo Write to log
				m_inputIo->service.run();
				std::cout << "IO task completed." << std::endl;
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		});
	m_thread = boost::thread(
		[&]() {
			try {
				m_log.Debug("Starting IO task...");
				m_io.run();
				m_log.Debug("IO task completed.");
			} catch (const trdk::EngineServer::Exception &ex) {
				m_log.Error(
					"Engine service stopped with error: \"%1%\".",
					ex.what());
				throw;
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		});

}

EngineServer::Service::~Service() {
	try {
		m_io.stop();
		m_thread.join();
		m_inputIo->service.stop();
		m_thread.join();
	} catch (...) {
		AssertFailNoException();
		throw;
	}
	m_io.reset();
	m_inputIo.reset();
}

void EngineServer::Service::Connect(const Lib::Ini &config) {

	const IniSectionRef section(config, "Service");
	const auto &host = section.ReadKey("host");
	const auto &port = section.ReadTypedKey<uint16_t>("port");
	m_log.Debug("Connecting to Trading Web Service at %1%:%2%...", host, port);

	m_socket.async_connect(
		io::ip::tcp::endpoint(io::ip::address::from_string(host), port),
		boost::bind(&Service::OnConnect, this, host, port, _1));

}

void EngineServer::Service::OnConnect(
		const std::string &host,
		uint16_t port,
		const boost::system::error_code &error) {
	
	if (error) {
		m_log.Error(
			"Failed to connect to Trading Web Service at %1%:%2%: \"%3%\".",
			host,
			port,
			SysError(error.value()));
		throw EngineServer::Exception("Failed to connect to Trading Web Service");
	}
	m_log.Debug("Connected to Trading Web Service at %1%:%2%, starting WAMP session...", host, port);

	m_startFuture = m_session->start().then(
		[this, host, port](boost::future<bool> isStarted) {

			if (!isStarted.get()) {
				throw EngineServer::Exception("Failed to start WAMP session");
			}

			m_log.Debug("WAMP session started, joining...");

			m_joinFuture = m_session->join("tws").then(
				[this, host, port](boost::future<uint64_t> realm) {
					m_log.Info(
						"Connected to Trading Web Service at %1%:%2%, realm \"%3%\".",
						host,
						port,
						realm.get());
					PublishEngine();
					ScheduleNextCurrentTimeNotification();
				});
	
		});

}

void EngineServer::Service::PublishEngine() {
	const auto &info = std::make_tuple(
		m_name,
		"acd51d02-896b-11e5-af63-feff819cdc9f",
		std::string(TRDK_BUILD_IDENTITY),
		m_suffix);
	m_session->publish(m_topics.onNewInstance, info);
	m_log.Info(
		"Published engine \"%1%\", build %2%, with suffix \"%3%\".",
		std::get<0>(info),
		std::get<1>(info),
		std::get<2>(info));
}

void EngineServer::Service::ScheduleNextCurrentTimeNotification() {

	if (!m_socket.is_open()) {
		return;
	}

	const boost::function<void(const boost::system::error_code &)> callback
		= [this] (const boost::system::error_code &error) {
			if (error) {
				return;
			}
			ScheduleNextCurrentTimeNotification();
			PublishCurrentTime();
		};

	Verify(m_currentTimePublishTimer.expires_from_now(pt::minutes(1)) == 0);
	m_currentTimePublishTimer.async_wait(boost::bind(callback, _1));

}

void EngineServer::Service::PublishCurrentTime() {
	m_session->publish(m_topics.time, pt::to_simple_string(m_log.GetTime()));
}

void EngineServer::Service::LoadEngine(const fs::path &configFilePath) {

	boost::shared_ptr<Settings> settings(
		new Settings(configFilePath, GetName(), *this));
	if (m_engines.find(settings->GetEngineId()) != m_engines.end()) {
		boost::format message("Engine with ID \"%1%\" already loaded");
		message % settings->GetEngineId();
		throw EngineServer::Exception(message.str().c_str());
	}

	m_engines[settings->GetEngineId()] = settings;
	//! @todo Write to log
	std::cout
		<< "Loaded engine \"" << settings->GetEngineId() << "\""
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
		settings.GetEngineId(),
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
	context.RaiseStateUpdate(Context::STATE_ENGINE_STARTED);
}

void EngineServer::Service::StopEngine(const std::string &engineId) {
	CheckEngineIdExists(engineId);
	m_server.StopAll(STOP_MODE_GRACEFULLY_ORDERS);
}

void EngineServer::Service::StartAccept() {
	const auto &newConnection = Client::Create(
		m_inputIo->acceptor.get_io_service(),
		*this);
    m_inputIo->acceptor.async_accept(
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
		const std::string *updateMessage) {
	boost::function<void(Client &)> fun;
	static_assert(Context::numberOfStates == 4, "List changed.");
	std::string message;
	switch (state) {
		case Context::STATE_ENGINE_STARTED:
			message = updateMessage
				?	(boost::format("Engine started: %1%.") % *updateMessage).str()
				:	"Engine started.";
			fun = [&message](Client &client) {
				client.SendMessage(message);
			};
			break;
		case Context::STATE_ENGINE_STOPPED_GRACEFULLY:
			message = updateMessage
				?	(boost::format("Engine stopped: %1%.") % *updateMessage).str()
				:	"Engine stopped.";
			fun = [&message](Client &client) {
				client.SendMessage(message);
			};
			break;
		case Context::STATE_ENGINE_STOPPED_ERROR:
			message = updateMessage
				?	(boost::format("Engine stopped with error: %1%.") % *updateMessage).str()
				:	"Engine stopped with error.";
			fun = [&message](Client &client) {
				client.SendError(message);
			};
			break;
		case Context::STATE_STRATEGY_BLOCKED:
			message = updateMessage
				?	(boost::format("Strategy blocked: %1%.") % *updateMessage).str()
				:	"Strategy blocked.";
			fun = [&message](Client &client) {
				client.SendError(message);
			};
			break;
		default:
			message = updateMessage
				?	(boost::format("Unknown context error: %1%.") % *updateMessage).str()
				:	"Unknown context error.";
			fun = [&message](Client &client) {
				client.SendError(message);
			};
			break;

	}
	const ConnectionsReadLock lock(m_connectionsMutex);
	foreach (Client *client, m_connections) {
		try {
			fun(*client);
		} catch (const trdk::Lib::Exception &ex) {
			m_log.Error("Failed to send message: \"%1%\".", ex);
		}
	}
}

void EngineServer::Service::ClosePositions(const std::string &engineId) {
	CheckEngineIdExists(engineId);
	m_server.ClosePositions();
}
