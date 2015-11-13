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

EngineServer::Service::Service(
		const std::string &name,
		const fs::path &engineConfigFilePath)
	: m_name(name)
	, m_suffix(boost::replace_all_copy(boost::to_lower_copy(m_name), ".", "_"))
	, m_topics(m_suffix)
	, m_settings(engineConfigFilePath, m_name, *this)
	, m_socket(m_io)
	, m_currentTimePublishTimer(m_io) {

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

	Connect(settings);
	
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
	} catch (...) {
		AssertFailNoException();
		throw;
	}
	m_io.reset();
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

/*void EngineServer::Service::StartEngine(const std::string &commandInfo) {
	Context &context = m_server.Run(
		m_settings.GetEngineId(),
		m_settings.GetFilePath(),
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

void EngineServer::Service::StopEngine() {
	m_server.StopAll(STOP_MODE_GRACEFULLY_ORDERS);
}s

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
*/
