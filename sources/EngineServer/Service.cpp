/**************************************************************************
 *   Created: 2015/03/17 01:13:15
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include <system_error>
#include "Prec.hpp"
#include "Service.hpp"
#include "Client.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::EngineServer;

namespace io = boost::asio;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace sys = boost::system;
namespace ab = autobahn;

////////////////////////////////////////////////////////////////////////////////

EngineServer::Service::Config::Config(const fs::path &configPath) {
	const IniFile ini(configPath);
	const IniSectionRef section(ini, "Service");
	name = section.ReadKey("name");
	id = section.ReadKey("id");
	twsHost = section.ReadKey("tws_host");
	twsPort = section.ReadTypedKey<uint16_t>("tws_port");
	timeout = pt::seconds(section.ReadTypedKey<size_t>("tws_timeout_seconds"));
	wampDebug = section.ReadTypedKey<bool>("wamp_debug");
}

////////////////////////////////////////////////////////////////////////////////

EngineServer::Service::Topics::Topics() {
	//...//
}

EngineServer::Service::Topics::Topics(uint64_t suffix)
	: time((boost::format("trdk.engine.%1%.time") % suffix).str()) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

EngineServer::Service::Connection::Connection(
		io::io_service &io,
		EventsLog &log,
		bool isWampDebugOn)
	: log(log)
	, socket(io)
	, currentTimePublishTimer(io)
	, ioTimeoutTimer(io)
	, session(std::make_shared<Session>(io, socket, socket, isWampDebugOn)) {
	//...//
}

void EngineServer::Service::Connection::ScheduleNextCurrentTimeNotification() {

	Verify(currentTimePublishTimer.expires_from_now(pt::seconds(1)) == 0);

	auto weakSelf = boost::weak_ptr<Connection>(shared_from_this());
	currentTimePublishTimer.async_wait(
		[weakSelf] (const sys::error_code &error) {
			auto sharedSelf = weakSelf.lock();
			if (!sharedSelf) {
				return;
			}
			if (error) {
				sharedSelf->log.Debug(
					"Engine time notification process stopped by"
						" \"%1%\" (%2%).",
					error.message(),
					error.value());
				return;
			}
			sharedSelf->ScheduleNextCurrentTimeNotification();
			const auto &now = sharedSelf->log.GetTime();
			sharedSelf->session->publish(
				sharedSelf->topics.time,
				std::make_tuple(pt::to_simple_string(now)));
		});

}

void EngineServer::Service::Connection::ScheduleIoTimeout(
		const pt::time_duration &timeout) {

	const auto &startTime = log.GetTime();
	Verify(ioTimeoutTimer.expires_from_now(timeout) == 0);

	auto weakSelf = boost::weak_ptr<Connection>(shared_from_this());
	ioTimeoutTimer.async_wait(
		[weakSelf, startTime, timeout] (const sys::error_code &error) {
			if (error) {
				return;
			}
			auto sharedSelf = weakSelf.lock();
			if (!sharedSelf) {
				return;
			}
			sharedSelf->log.Warn(
				"TWS IO operation time out: %1% -> %2% (%3%).",
				startTime,
				sharedSelf->log.GetTime(),
				timeout);
			sharedSelf->socket.close();
			throw TimeoutException("TWS IO operation time out");
		});

}

void EngineServer::Service::Connection::StopIoTimeout() {
	ioTimeoutTimer.cancel();
}

////////////////////////////////////////////////////////////////////////////////

EngineServer::Service::InputIo::InputIo()
	:	acceptor(
			service,
			io::ip::tcp::endpoint(io::ip::tcp::v4(), 3689)) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

EngineServer::Service::Service(
		const std::string &name,
		const fs::path &engineConfigFilePath)
	: m_config(engineConfigFilePath)
	, m_settings(engineConfigFilePath, m_config.name, *this)
	, m_inputIo(new InputIo)
	, m_numberOfReconnects(0)
	, m_io(new io::io_service)
	, m_isInited(false) {

	m_log.EnableStdOut();

	const IniFile settings(m_settings.GetFilePath());

	const auto logFilePath = settings.ReadFileSystemPath("General", "logs_dir")
		/ "service.log";
	fs::create_directories(logFilePath.branch_path());
	m_logFile.open(
		logFilePath.string().c_str(),
		std::ios::out | std::ios::ate | std::ios::app);
	if (!m_logFile) {
		m_log.Error("Failed to open file %1% for logging.", logFilePath);
		throw EngineServer::Exception("Failed to open file for logging");
	}
	m_log.EnableStream(m_logFile, true);

	m_log.Info(
		"Loaded engine \"%1%\" from %2% for engine service \"%3%\".",
		m_settings.GetEngineId(),
		m_settings.GetFilePath(),
		m_config.name);

	{
		const auto &endpoint = m_inputIo->acceptor.local_endpoint();
		m_log.Info(
			"Opening endpoint \"%1%:%2%\" for incoming connections...",
			endpoint.address(),
			endpoint.port());
	}

	StartAccept();
	m_inputIoThread = boost::thread(
		[&]() {
			try {
				m_log.Debug("Starting Input IO task...");
				m_inputIo->service.run();
				m_log.Debug("Input IO task completed");
			} catch (...) {
				AssertFailNoException();
				throw;
			}
		});

	//! @todo Legacy support, remove try-catch.
	try {
		ConnectionLock lock(m_connectionMutex);
		auto connection = Connect();
		StartIoThread();
		m_connectionCondition.wait(lock);
		bool isInited = false;
		try {
			isInited
				= connection->sessionStartFuture.timed_wait(m_config.timeout)
				&& connection->sessionJoinFuture.timed_wait(m_config.timeout)
				&& connection->engineRegistrationFuture.timed_wait(m_config.timeout)
				&& m_isInited;
		} catch (const std::exception &ex) {
			//...//
		}
		if (!isInited) {
			throw Exception("Failed to start TWS session");
		}
	} catch (...) {
		try {
			m_inputIo->service.stop();
			m_inputIoThread.join();
			m_inputIo.reset();
		} catch (...) {
			AssertFailNoException();
		}
		throw;
	}

	m_log.Info("Engine service started.");

}

EngineServer::Service::~Service() {
	try {
		m_io->stop();
		m_thread.join();
		m_inputIo->service.stop();
		m_inputIoThread.join();
	} catch (...) {
		AssertFailNoException();
		throw;
	}
	m_inputIo.reset();
}

boost::shared_ptr<EngineServer::Service::Connection> EngineServer::Service::Connect() {
	m_log.Debug(
		"Connecting to TWS at %1%:%2%...",
		m_config.twsHost,
		m_config.twsPort);
	auto connection = boost::make_shared<Connection>(
		*m_io,
		m_log,
		m_config.wampDebug);
	connection->ScheduleIoTimeout(m_config.timeout);
	connection->socket.async_connect(
		io::ip::tcp::endpoint(
			io::ip::address::from_string(m_config.twsHost),
			m_config.twsPort),
		[this, connection](const sys::error_code &error) {
			connection->StopIoTimeout();
			OnConnected(connection, error);
		});
	return connection;
}

void EngineServer::Service::OnConnected(
		boost::shared_ptr<Connection> connection,
		const sys::error_code &error) {

	if (error) {
		m_log.Error(
			"Failed to connect to TWS at %1%:%2%: \"%3%\" (%4%).",
			m_config.twsHost,
			m_config.twsPort,
			error.message(),
			error.value());
		throw ConnectError("Failed to connect to TWS");
	}
	m_log.Debug("Connected to TWS, starting session...");

	connection->ScheduleIoTimeout(m_config.timeout);
	connection->sessionStartFuture = connection->session->start().then(
		[this, connection](boost::future<bool> isStartedFuture) {
			connection->StopIoTimeout();
			bool isStarted;
			try {
				isStarted = isStartedFuture.get();
			} catch (const std::exception &ex) {
				m_log.Error(
					"Failed to start TWS session at \"%1%:%2%\": \"%3%\".",
					m_config.twsHost,
					m_config.twsPort,
					ex.what());
			}
			m_io->post(
				[this, connection, isStarted]() {
					OnSessionStarted(connection, isStarted);
				});
		});

}

void EngineServer::Service::OnSessionStarted(
		boost::shared_ptr<Connection> connection,
		bool isStarted) {

	if (!isStarted) {
		throw ConnectError("TWS session not started");
	}
	m_log.Debug("TWS session started, joining...");

	connection->ScheduleIoTimeout(m_config.timeout);
	connection->sessionJoinFuture
		= connection->session->join("tws").then(
			[this, connection](boost::future<uint64_t> sessionIdFuture) {
				connection->StopIoTimeout();
				boost::optional<uint64_t> sessionId;
				try {
					sessionId = sessionIdFuture.get();
				} catch (const std::exception &ex) {
					m_log.Error(
						"Failed to join to TWS session at \"%1%:%2%\": \"%3%\".",
						m_config.twsHost,
						m_config.twsPort,
						ex.what());
				}
				m_io->post(
					[this, connection, sessionId]() {
						OnSessionJoined(connection, sessionId);
					});
			});

}

void EngineServer::Service::OnSessionJoined(
		boost::shared_ptr<Connection> connection,
		const boost::optional<uint64_t> &sessionId) {

	if (!sessionId) {
		throw ConnectError("Failed to join to TWS session");
	}
	m_log.Info(
		"Joined to TWS session %1% at \"%2%:%3%\".",
		*sessionId,
		m_config.twsHost,
		m_config.twsPort);

	const auto &engineInfo = std::make_tuple(
		m_config.id,
		m_config.name,
		std::string(TRDK_BUILD_IDENTITY));

	connection->ScheduleIoTimeout(m_config.timeout);
	connection->engineRegistrationFuture
		= connection
		->session
		->call("trdk.service.engine.register", engineInfo)
		.then(
			[this, connection, engineInfo](
					boost::future<ab::wamp_call_result> callResult) {
				connection->StopIoTimeout();
				boost::optional<uint64_t> instanceId;
				try {
					instanceId = callResult.get().argument<uint64_t>(0);
				} catch (const std::exception &ex) {
					m_log.Error(
						"Failed to register TWS engine instance \"%1% %2%\""
							" with ID \"%3%\": \"%4%\".",
						std::get<1>(engineInfo),
						std::get<2>(engineInfo),
						std::get<0>(engineInfo),
						ex.what());
				}
				m_io->post(
					[this, connection, instanceId, engineInfo]() {
						OnEngineRegistered(connection, instanceId, engineInfo);
					});
			});

}

void EngineServer::Service::OnEngineRegistered(
		boost::shared_ptr<Connection> connection,
		const boost::optional<uint64_t> &instanceId,
		const std::tuple<std::string, std::string, std::string> &engineInfo) {

	if (!instanceId) {
		throw ConnectError("Failed to register TWS engine instance");
	}
	m_log.Info(
		"Registered TWS engine instance %1%"
			" for engine \"%2% %3%\" with ID \"%4%\".",
		*instanceId,
		std::get<1>(engineInfo),
		std::get<2>(engineInfo),
		std::get<0>(engineInfo));

	connection->topics = Topics(*instanceId);
	connection->ScheduleNextCurrentTimeNotification();

	{
		const ConnectionLock lock(m_connectionMutex);
		m_numberOfReconnects = 0;
		connection.swap(m_connection);
		m_isInited = true;
	}
	m_connectionCondition.notify_all();

}

void EngineServer::Service::Reconnect() {
	AssertEq(0, m_numberOfReconnects);
	{
		const ConnectionLock lock(m_connectionMutex);
		m_connection.reset();
	}
	Connect();
}

void EngineServer::Service::RepeatReconnection(
		const Exception &prevReconnectError) {

	++m_numberOfReconnects;

	{
		const ConnectionLock lock(m_connectionMutex);
		m_connection.reset();
	}

	auto timer = boost::make_shared<io::deadline_timer>(*m_io);
	const auto &sleepTime
		= pt::seconds(long(std::min<long>(m_numberOfReconnects, 30)));
	timer->expires_from_now(sleepTime);

	m_log.Warn(
		"Failed to reconnect: \"%1%\". Trying again in %2% (%3% times)...",
		prevReconnectError.what(),
		sleepTime,
		m_numberOfReconnects);

	timer->async_wait(
		[this, timer] (const sys::error_code &error) {
			if (error) {
				m_log.Debug(
					"Reconnect canceled: \"%1%\" (%2%).",
					error.message(),
					error.value());
				return;
			}
			Connect();
		});

}

void EngineServer::Service::StartIoThread() {

	m_thread = boost::thread(
		[this]() {

			m_log.Debug("Started TWS IO task.");

			for ( ; ; ) {
				try {
					m_io->run();
					break;
				} catch (const trdk::EngineServer::Service::Exception &ex) {
					m_connectionCondition.notify_all();
					if (!m_isInited) {
						break;
					}
					m_io.reset(new io::io_service);
					RepeatReconnection(ex);
				} catch (const trdk::EngineServer::Exception &ex) {
					m_log.Error(
						"TWS IO task stopped with error: \"%1%\".",
						ex.what());
					m_connectionCondition.notify_all();
					throw;
				} catch (const sys::system_error &ex) {
					m_log.Error(
						"TWS IO task stopped with error: \"%1%\" (%2%).",
						ex.code().message(),
						ex.code().value());
					m_connectionCondition.notify_all();
					if (ex.code().value() != sys::errc::broken_pipe) {
						throw;
					} else if (!m_isInited) {
						break;
					}
					m_io.reset(new io::io_service);
					Reconnect();
				} catch (...) {
					AssertFailNoException();
					throw;
				}
			}

			m_log.Debug("TWS IO task completed.");

		});

}

void EngineServer::Service::ForEachEngineId(
		const boost::function<void(const std::string &engineId)> &func)
		const {
	func(m_settings.GetEngineId());
}

bool EngineServer::Service::IsEngineStarted(const std::string &engineId) const {
	CheckEngineIdExists(engineId);
	return m_server.IsStarted(engineId);
}

EngineServer::Settings & EngineServer::Service::GetEngineSettings(
		const std::string &engineId) {
	CheckEngineIdExists(engineId);
	// return m_engines[engineId];
	return m_settings;
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
		const sys::error_code &error) {
	if (!error) {
		const ConnectionsWriteLock lock(m_connectionsMutex);
		auto list(m_connections);
		list.insert(&*newConnection);
		newConnection->Start();
		list.swap(m_connections);
	} else {
		m_log.Error(
			"Failed to accept new client: \"%1%\".",
			SysError(error.value()));
	}
	StartAccept();
}

void EngineServer::Service::OnDisconnect(Client &client) {
	const ConnectionsWriteLock lock(m_connectionsMutex);
	m_connections.erase(&client);
}

void EngineServer::Service::CheckEngineIdExists(const std::string &id) const {
	if (m_settings.GetEngineId() != id) {
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

////////////////////////////////////////////////////////////////////////////////
