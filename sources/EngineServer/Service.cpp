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
#include "Pack.hpp"
#include "Core/Strategy.hpp"
#include "Core/PriceBook.hpp"
#include "Core/MarketDataSource.hpp"

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::EngineServer;

namespace io = boost::asio;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace sys = boost::system;
namespace uuids = boost::uuids;
namespace ab = autobahn;

////////////////////////////////////////////////////////////////////////////////

EngineServer::Service::Config::Config(const fs::path &configPath)
		: path(configPath) {
	const IniFile ini(path);
	const IniSectionRef section(ini, "Service");
	name = section.ReadKey("name");
	id = section.ReadKey("id");
	twsHost = section.ReadKey("tws_host");
	twsPort = section.ReadTypedKey<uint16_t>("tws_port");
	timeout = pt::seconds(
		long(section.ReadTypedKey<size_t>("tws_timeout_seconds")));
	wampDebug = section.ReadTypedKey<bool>("wamp_debug");
	callOptions.set_timeout(
		std::chrono::milliseconds(timeout.total_milliseconds()));
}

////////////////////////////////////////////////////////////////////////////////

EngineServer::Service::Topics::Topics(const std::string &suffix)
	: registerEngine("trdk.service.engine.register")
	, onEngineConnected("trdk.engine.on_connected")
	, time((boost::format("trdk.engine.%1%.time") % suffix).str())
	, state((boost::format("trdk.engine.%1%.state") % suffix).str())
	, storeOperationStart("trdk.service.operation.store_start")
	, storeOperationEnd("trdk.service.operation.store_end")
	, storeOrder("trdk.service.order.store")
	, storeTrade("trdk.service.trade.store")
	, storeBook("trdk.service.book.store")
	, storeBar("trdk.service.bar.store")
	, registerAbstractDataSource("trdk.service.abstract_data_source.register")
	, storeAbstractDataPoint("trdk.service.abstract_data.store")
	, startEngine((boost::format("trdk.engine.%1%.start") % suffix).str())
	, stopEngine((boost::format("trdk.engine.%1%.stop") % suffix).str())
	, startDropCopy(
		(boost::format("trdk.engine.%1%.drop_copy.start") % suffix).str())
	, stopDropCopy(
		(boost::format("trdk.engine.%1%.drop_copy.stop") % suffix).str())
	, closePositions(
		(boost::format("trdk.engine.%1%.close_positions") % suffix).str()) {
	//...//
}

////////////////////////////////////////////////////////////////////////////////

EngineServer::Service::Connection::Connection(
		io::io_service &io,
		const Topics &topics,
		EventsLog &log,
		bool isWampDebugOn)
	: log(log)
	, topics(topics)
	, socket(io)
	, currentTimePublishTimer(io)
	, ioTimeoutTimer(io)
	, session(std::make_shared<Session>(io, socket, socket, isWampDebugOn)) {
	//...//
}

void EngineServer::Service::Connection::ScheduleNextCurrentTimeNotification() {

#	ifdef _DEBUG
		const auto period = pt::minutes(1);
#	else
		const auto period = pt::seconds(1);
#	endif

	Verify(currentTimePublishTimer.expires_from_now(period) == 0);

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
				std::make_tuple(ConvertToTimeT(now)));
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

EngineServer::Service::OrderCache::OrderCache(
		const uuids::uuid &id,
		const std::string *tradingSystemId,
		const pt::ptime *orderTime,
		const pt::ptime *executionTime,
		const OrderStatus &status,
		const uuids::uuid &operationId,
		const int64_t *subOperationId,
		const Security &security,
		const TradingSystem &tradingSystem,
		const OrderSide &side,
		const Qty &qty,
		const double *price,
		const TimeInForce *timeInForce,
		const Currency &currency,
		const Qty *minQty,
		const Qty &executedQty,
		const double *bestBidPrice,
		const Qty *bestBidQty,
		const double *bestAskPrice,
		const Qty *bestAskQty)
	: id(id)
	, status(status)
	, operationId(operationId)
	, security(&security)
	, tradingSystem(&tradingSystem)
	, side(side)
	, qty(qty)
	, currency(currency)
	, executedQty(executedQty) {
	if (tradingSystemId) {
		this->tradingSystemId = *tradingSystemId;
	}
	if (orderTime) {
		this->orderTime = *orderTime;
	}
	if (executionTime) {
		this->executionTime = *executionTime;
	}
	if (subOperationId) {
		this->subOperationId = *subOperationId;
	}
	if (price) {
		this->price = *price;
	}
	if (timeInForce) {
		this->timeInForce = *timeInForce;
	}
	if (minQty) {
		this->minQty = *minQty;
	}
	if (bestBidPrice) {
		this->bestBidPrice = *bestBidPrice;
	}
	if (bestBidQty) {
		this->bestBidQty = *bestBidQty;
	}
	if (bestAskPrice) {
		this->bestAskPrice = *bestAskPrice;
	}
	if (bestAskQty) {
		this->bestAskQty = *bestAskQty;
	}
}

////////////////////////////////////////////////////////////////////////////////

EngineServer::Service::DropCopy::DropCopy(Service &service)
	: m_service(service)
	, m_log(m_service.m_log)
	, m_queue(m_log)
	, m_isSecondaryDataDisabled(false) {
	//...//
}

EngineServer::Service::DropCopy::~DropCopy() {
	//...//
}

void EngineServer::Service::DropCopy::OpenDataLog(const fs::path &logsDir) {

	if (m_dataLogFile.is_open()) {
		throw LogicError("Drop Copy log already is opened");
	}

	const auto dataLogFilePath = logsDir / "dropcopy.log";
	m_dataLogFile.open(
		dataLogFilePath.string().c_str(),
		std::ios::out | std::ios::ate | std::ios::app);
	if (!m_dataLogFile) {
		m_log.Error(
			"Failed to open file %1% for Drop Copy logging.",
			dataLogFilePath);
		throw Exception("Failed to open file for Drop Copy logging");
	}

	m_dataLog.EnableStream(m_dataLogFile, true);

	m_log.Info("Logging Drop Copy into %1%.", dataLogFilePath);

}

void EngineServer::Service::DropCopy::OnConnectionRestored() {
	m_queue.OnConnectionRestored();
}

void EngineServer::Service::DropCopy::Start() {
	m_queue.Start();
}

void EngineServer::Service::DropCopy::Stop(bool sync) {
	m_queue.Stop(sync);
}

bool EngineServer::Service::DropCopy::IsStarted() const {
	return m_queue.IsStarted();
}

void EngineServer::Service::DropCopy::Flush() {
	m_queue.Flush();
}

void EngineServer::Service::DropCopy::Dump() {
	m_queue.Dump();
}

void EngineServer::Service::DropCopy::CopyOrder(
		const uuids::uuid &id,
		const std::string *tradingSystemId,
		const pt::ptime *orderTime,
		const pt::ptime *executionTime,
		const OrderStatus &status,
		const uuids::uuid &operationId,
		const int64_t *subOperationId,
		const Security &security,
		const TradingSystem &tradingSystem,
		const OrderSide &side,
		const Qty &qty,
		const double *price,
		const TimeInForce *timeInForce,
		const Currency &currency,
		const Qty *minQty,
		const Qty &executedQty,
		const double *bestBidPrice,
		const Qty *bestBidQty,
		const double *bestAskPrice,
		const Qty *bestAskQty) {
	const OrderCache order(
		id,
		tradingSystemId,
		orderTime,
		executionTime,
		status,
		operationId,
		subOperationId,
		security,
		tradingSystem,
		side,
		qty,
		price,
		timeInForce,
		currency,
		minQty,
		executedQty,
		bestBidPrice,
		bestBidQty,
		bestAskPrice,
		bestAskQty);
	m_queue.Enqueue(
		[this, order](size_t recordIndex, size_t attemptNo, bool dump) -> bool {
			return m_service.StoreOrder(recordIndex, attemptNo, dump, order);
		});
}

void EngineServer::Service::DropCopy::CopyTrade(
		const pt::ptime &time,
		const std::string &tradingSystemTradeId,
		const uuids::uuid &orderId,
		double price,
		const Qty &qty,
		double bestBidPrice,
		const Qty &bestBidQty,
		double bestAskPrice,
		const Qty &bestAskQty) {
	m_queue.Enqueue(
		[
					this,
					time,
					tradingSystemTradeId,
					orderId,
					price,
					qty,
					bestBidPrice,
					bestBidQty,
					bestAskPrice,
					bestAskQty](
				size_t recordIndex,
				size_t attemptNo,
				bool dump)
				-> bool {
			return m_service.StoreTrade(
				recordIndex,
				attemptNo,
				dump,
				time,
				tradingSystemTradeId,
				orderId,
				price,
				qty,
				bestBidPrice,
				bestBidQty,
				bestAskPrice,
				bestAskQty);
		});
}

void EngineServer::Service::DropCopy::ReportOperationStart(
		const uuids::uuid &id,
		const pt::ptime &time,
		const Strategy &strategy) {
	m_queue.Enqueue(
		[this, id, time, &strategy](
				size_t recordIndex,
				size_t attemptNo,
				bool dump)
				-> bool {
			return m_service.StoreOperationStartReport(
				recordIndex,
				attemptNo,
				dump,
				id,
				time,
				strategy);
		});
}

void EngineServer::Service::DropCopy::ReportOperationEnd(
		const uuids::uuid &id,
		const pt::ptime &time,
		const OperationResult &result,
		double pnl,
		const boost::shared_ptr<const FinancialResult> &financialResult) {
	m_queue.Enqueue(
		[this, id, time, result, pnl, financialResult](
				size_t recordIndex,
				size_t attemptNo,
				bool dump)
				-> bool {
			return m_service.StoreOperationEndReport(
				recordIndex,
				attemptNo,
				dump,
				id,
				time,
				result,
				pnl,
				financialResult);
		});
}

void EngineServer::Service::DropCopy::CopyBook(
		const Security &security,
		const PriceBook &book) {

#	ifdef DEV_VER
		const auto &maxQueueSize = 1000;
#	else
		const auto &maxQueueSize = 100000;
#	endif
	const auto &queueSize = m_queue.GetSize();
	if (queueSize > maxQueueSize) {
		if (!m_isSecondaryDataDisabled) {
			m_isSecondaryDataDisabled = true;
			m_log.Error(
				"Secondary Drop Copy data is disabled - queue is too big"
					" (queue size %1% is greater than maximum allowed %2%).",
				queueSize,
				maxQueueSize);
		}
		return;
	} else if (m_isSecondaryDataDisabled) {
		m_isSecondaryDataDisabled = false;
		m_log.Info(
			"Secondary Drop Copy data is enabled again (queue size is %1%).",
			queueSize);
	}

	m_queue.Enqueue(
		[this, &security, book](
				size_t recordIndex,
				size_t attemptNo,
				bool dump)
				-> bool {
			return m_service.StoreBook(
				recordIndex,
				attemptNo,
				dump,
				security,
				book);
		});

}

void EngineServer::Service::DropCopy::CopyBar(
		const Security &security,
		const pt::ptime &time,
		const pt::time_duration &size,
		const ScaledPrice &openTradePrice,
		const ScaledPrice &closeTradePrice,
		const ScaledPrice &highTradePrice,
		const ScaledPrice &lowTradePrice) {
	const BarCache bar = {
		&security,
		time,
		size,
		openTradePrice,
		closeTradePrice,
		highTradePrice,
		lowTradePrice
	};
	m_queue.Enqueue(
		[this, bar](size_t recordIndex, size_t attemptNo, bool dump) -> bool {
			return m_service.StoreBar(recordIndex, attemptNo, dump, bar);
		});
}

EngineServer::Service::DropCopy::AbstractDataSourceId
EngineServer::Service::DropCopy::RegisterAbstractDataSource(
		const uuids::uuid &instance,
		const uuids::uuid &type,
		const std::string &name) {
	return m_service.RegisterAbstractDataSource(instance, type, name);
}

void EngineServer::Service::DropCopy::CopyAbstractDataPoint(
		const AbstractDataSourceId &source,
		const pt::ptime &time,
		double value) {
	m_queue.Enqueue(
		[this, source, time, value](
				size_t recordIndex,
				size_t attemptNo,
				bool dump)
				-> bool {
			return m_service.StoreAbstractDataPoint(
				recordIndex,
				attemptNo,
				dump,
				source,
				time,
				value);
		});
}

////////////////////////////////////////////////////////////////////////////////

EngineServer::Service::Service(
		const fs::path &engineConfigFilePath,
		const pt::time_duration &startDelay)
	: m_config(engineConfigFilePath)
	, m_engineState(ENGINE_STATE_STOPPED)
	, m_topics(m_config.id)
	, m_numberOfReconnects(0)
	, m_io(new io::io_service)
	, m_dropCopy(*this) {

	m_log.EnableStdOut();

	const IniFile settings(m_config.path);

	const auto logsDir = settings.ReadFileSystemPath("General", "logs_dir");
	fs::create_directories(logsDir);

	const auto logFilePath = logsDir / "service.log";
	m_logFile.open(
		logFilePath.string().c_str(),
		std::ios::out | std::ios::ate | std::ios::app);
	if (!m_logFile) {
		m_log.Error("Failed to open file %1% for logging.", logFilePath);
		throw Exception("Failed to open file for logging");
	}
	m_log.EnableStream(m_logFile, true);

	m_dropCopy.OpenDataLog(logsDir);
	m_dropCopy.Start();

	m_log.Info(
		"Loaded engine from %1% for engine service \"%2%\".",
		m_config.path,
		m_config.name);

	{
		ConnectionLock lock(m_connectionMutex);
		auto connection = Connect(startDelay);
		RunIoThread();
		m_connectionCondition.wait(lock);
		bool isInited = false;
		try {
			isInited
				= connection->sessionStartFuture.timed_wait(m_config.timeout)
				&& connection->sessionJoinFuture.timed_wait(m_config.timeout)
				&& connection->engineRegistrationFuture.timed_wait(m_config.timeout)
				&& m_instanceId;
		} catch (const std::exception &) {
			//...//
		}
		if (!isInited) {
			throw Exception("Failed to start TWS session");
		}
	}

	m_log.Info("Engine service started.");

}

EngineServer::Service::~Service() {
	try {
		m_dropCopyTask.Join();
		m_engineTask.Join();
		m_io->stop();
		m_thread.join();
	} catch (...) {
		AssertFailNoException();
		terminate();
	}
}

boost::shared_ptr<EngineServer::Service::Connection> EngineServer::Service::Connect(
		const pt::time_duration &startDelay) {
	m_log.Debug(
		"Connecting to TWS at %1%:%2%...",
		m_config.twsHost,
		m_config.twsPort);
	auto connection = boost::make_shared<Connection>(
		*m_io,
		m_topics,
		m_log,
		m_config.wampDebug);
	connection->ScheduleIoTimeout(m_config.timeout);
	connection->socket.async_connect(
		io::ip::tcp::endpoint(
			io::ip::address::from_string(m_config.twsHost),
			m_config.twsPort),
		[this, connection, startDelay](const sys::error_code &error) {
			connection->StopIoTimeout();
			OnConnected(connection, error, startDelay);
		});
	return connection;
}

void EngineServer::Service::OnConnected(
		const boost::shared_ptr<Connection> &connection,
		const sys::error_code &error,
		const pt::time_duration &startDelay) {

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
		[this, connection, startDelay](boost::future<bool> isStartedFuture) {
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
				[this, connection, isStarted, startDelay]() {
					OnSessionStarted(connection, isStarted, startDelay);
				});
		});

}

void EngineServer::Service::OnSessionStarted(
		boost::shared_ptr<Connection> connection,
		bool isStarted,
		const pt::time_duration &startDelay) {

	if (!isStarted) {
		throw ConnectError("TWS session not started");
	}
	m_log.Debug("TWS session started, joining...");

	connection->ScheduleIoTimeout(m_config.timeout);
	connection->sessionJoinFuture
		= connection->session->join("tws").then(
			[this, connection, startDelay](
					boost::future<uint64_t> sessionIdFuture) {
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
					[this, connection, sessionId, startDelay]() {
						OnSessionJoined(connection, sessionId, startDelay);
					});
			});

}

void EngineServer::Service::OnSessionJoined(
		const boost::shared_ptr<Connection> &connection,
		const boost::optional<uint64_t> &sessionId,
		const pt::time_duration &startDelay) {

	if (!sessionId) {
		throw ConnectError("Failed to join to TWS session");
	}
	m_log.Info(
		"Joined to TWS session %1% at \"%2%:%3%\".",
		*sessionId,
		m_config.twsHost,
		m_config.twsPort);
	if (!startDelay.is_not_a_date_time()) {
		m_log.Info(
			"Waiting %1% for the engine instance registration...",
			startDelay);
		boost::this_thread::sleep(startDelay);
		m_log.Debug("Registering...");
	}

	const auto &engineInfo = std::make_tuple(
		m_config.id,
		m_config.name,
		std::string(TRDK_BUILD_IDENTITY));

	connection->ScheduleIoTimeout(m_config.timeout);
	connection->engineRegistrationFuture
		= connection
		->session
		->call(
			connection->topics.registerEngine,
			engineInfo,
			m_config.callOptions)
		.then(
			[this, connection, sessionId, engineInfo](
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
					[this, connection, sessionId, instanceId, engineInfo]() {
						OnEngineRegistered(
							connection,
							*sessionId,
							instanceId,
							engineInfo);
					});
			});

}

void EngineServer::Service::OnEngineRegistered(
		boost::shared_ptr<Connection> connection,
		uint64_t sessionId,
		const boost::optional<uint64_t> &instanceId,
		const std::tuple<std::string, std::string, std::string> &engineInfo) {

	if (!instanceId) {
		throw ConnectError("Failed to register TWS engine instance");
	} else if (m_instanceId && *m_instanceId != *&instanceId) {
		throw ConnectError(
			"Failed to register TWS engine instance"
				": Engine Instance ID is changed");
	}

	RegisterMethods(*connection);

	const auto engineState = m_engineState.load();
	connection->session->publish(
		connection->topics.onEngineConnected,
		std::make_tuple(
			m_config.id,
			*instanceId,
			m_config.name,
			std::string(TRDK_BUILD_IDENTITY),
			int(engineState),
			sessionId));
	connection->ScheduleNextCurrentTimeNotification();

	m_dropCopy.OnConnectionRestored();

	m_log.Info(
		"Registered TWS engine instance %1%"
			" for engine \"%2% %3%\" with ID \"%4%\".",
		*instanceId,
		std::get<1>(engineInfo),
		std::get<2>(engineInfo),
		std::get<0>(engineInfo));

	{
		const ConnectionLock lock(m_connectionMutex);
		m_numberOfReconnects = 0;
		connection.swap(m_connection);
		m_instanceId = *instanceId;
	}
	m_connectionCondition.notify_all();

	try {
		if (engineState != m_engineState) {
			PublishState();
		}
	} catch (...) {
		AssertFailNoException();
	}

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
		= pt::seconds(std::min(long(m_numberOfReconnects), long(30)));
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

void EngineServer::Service::RunIoThread() {

	m_thread = boost::thread(
		[this]() {

			m_log.Debug("Started TWS IO task.");

			for ( ; ; ) {
				try {
					m_io->run();
					break;
				} catch (const trdk::EngineServer::Service::TimeoutException &ex) {
					if (!m_instanceId) {
						break;
					}
					m_io.reset(new io::io_service);
					RepeatReconnection(ex);
				} catch (const trdk::EngineServer::Service::Exception &ex) {
					m_log.Error("TWS IO error: \"%1%\".", ex.what());
					m_connectionCondition.notify_all();
					if (!m_instanceId) {
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
					switch (ex.code().value()) {
						case sys::errc::connection_aborted:
						case sys::errc::connection_already_in_progress:
						case sys::errc::connection_refused:
						case sys::errc::connection_reset:
						case sys::errc::broken_pipe:
							break;
						default:
							throw;
					}
					if (!m_instanceId) {
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

void EngineServer::Service::RegisterMethods(Service::Connection &connection) {

	connection.session->provide(
		connection.topics.startEngine,
		[this](const autobahn::wamp_invocation &) {StartEngine();});
	connection.session->provide(
		connection.topics.stopEngine,
		[this](const autobahn::wamp_invocation &) {StopEngine();});

	connection.session->provide(
		connection.topics.startDropCopy,
		[this](const autobahn::wamp_invocation &) {StartDropCopy();});
	connection.session->provide(
		connection.topics.stopDropCopy,
		[this](const autobahn::wamp_invocation &) {StopDropCopy();});

	connection.session->provide(
		connection.topics.closePositions,
		[this](const autobahn::wamp_invocation &) {ClosePositions();});

}

void EngineServer::Service::PublishState() const {
	const ConnectionLock lock(m_connectionMutex);
	if (!m_connection) {
		// State publishes each time at reconnect.
		return;
	}
	m_connection->session->publish(
		m_connection->topics.state,
		std::make_tuple(int(m_engineState)));
}

void EngineServer::Service::StartEngine() {

	m_log.Debug("Starting engine...");

	const EngineLock lock(m_engineMutex);

	if (m_engineTask.IsActive()) {
		m_log.Warn(
			"Failed to start engine"
				" - service performs task which can change engine state.");
		throw Exception("Service performs task which can change engine state");
	} else if (m_engine) {
		m_log.Warn("Failed to start engine - engine already is started.");
		throw Exception("Engine already is started");
	}
	AssertEq(ENGINE_STATE_STOPPED, m_engineState);

	if (!m_dropCopy.IsStarted()) {
		m_log.Error("Failed to start engine - Drop Copy is not started.");
		throw Exception("Drop Copy is not started");
	}

	m_engineTask.Start(
		[this]() {
			m_log.Debug("Started engine start task.");
			try {
				auto engine = boost::make_shared<Engine>(
					m_config.path,
						[this](
							const Context::State &state,
							const std::string *updateMessage) {
						OnContextStateChanged(state, updateMessage);
					},
					m_dropCopy,
					false);
				{
					const EngineLock lock(m_engineMutex);
					Assert(!m_engine);
					if (m_engineState != ENGINE_STATE_STARTED) {
						m_log.Warn(
							"Failed to start engine. Engine state is %1%.",
							m_engineState);
						return;
					}
					engine.swap(m_engine);
				}
			} catch (const trdk::Lib::Exception &ex) {
				m_log.Warn("Failed to start engine: \"%1%\".", ex);
				m_engineState = ENGINE_STATE_STOPPED;
				try {
					PublishState();
				} catch (...) {
					AssertFailNoException();
					throw;
				}
				return;
			} catch (...) {
				AssertFailNoException();
				throw;
			}
			m_log.Info("Engine started.");
		});

	m_engineState = ENGINE_STATE_STARTING;
	try {
		PublishState();
	} catch (...) {
		AssertFailNoException();
		throw;
	}

}

void EngineServer::Service::StopEngine() {

	m_log.Debug("Stopping engine...");

	const EngineLock lock(m_engineMutex);

	if (m_engineTask.IsActive()) {
		m_log.Warn(
			"Failed to stop engine"
				" - service performs task which can change engine state.");
		throw Exception("Service performs task which can change engine state");
	} else if (!m_engine) {
		m_log.Warn("Failed to stop engine - engine already is stopped.");
		throw Exception("Engine already is stopped");
	}
	AssertEq(ENGINE_STATE_STARTED, m_engineState);

	m_engineTask.Start(
		[this]() {
			m_log.Debug("Started engine stop task.");
			try {
				m_engine->Stop(STOP_MODE_IMMEDIATELY);
				const EngineLock lock(m_engineMutex);
				m_engine.reset();
				AssertEq(ENGINE_STATE_STOPPING, m_engineState);
				m_engineState = ENGINE_STATE_STOPPED;
				PublishState();
				m_bookSentTime.clear();
			} catch (...) {
				AssertFailNoException();
				throw;
			}
			m_log.Info("Engine stopped.");
		});

	m_engineState = ENGINE_STATE_STOPPING;
	try {
		PublishState();
	} catch (...) {
		AssertFailNoException();
		throw;
	}

}

void EngineServer::Service::StartDropCopy() {
	m_log.Debug("Starting Drop Copy...");
	try {
		m_dropCopy.Start();
	} catch (const std::exception &ex) {
		m_log.Warn("Failed to start Drop Copy: \"%1%\".", ex.what());
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
	m_log.Info("Drop Copy strated.");
}

void EngineServer::Service::StopDropCopy() {
	m_log.Debug("Stopping Drop Copy...");
	const EngineLock lock(m_engineMutex);
	if (m_dropCopyTask.IsActive()) {
		m_log.Warn(
			"Failed to stop Drop Copy"
				" - service performs task which can change Drop Copy state.");
		throw Exception(
			"Service performs task which can change Drop Copy state");
	}
	try {
		m_dropCopy.Stop(false);
		m_dropCopyTask.Start(
			[this]() {
				m_log.Debug("Started Drop Copy stop-task.");
				try {
					m_dropCopy.Stop(true);
				} catch (...) {
					AssertFailNoException();
					throw;
				}
				m_log.Info("Drop Copy stopped.");
			});
	} catch (const std::exception &ex) {
		m_log.Warn("Failed to stop Drop Copy: \"%1%\".", ex.what());
		throw;
	} catch (...) {
		AssertFailNoException();
		throw;
	}
}

void EngineServer::Service::ClosePositions() {

	m_log.Debug("Closing positions...");

	const EngineLock lock(m_engineMutex);

	if (m_engineTask.IsActive()) {
		m_log.Warn(
			"Failed to close positions"
				" - service performs task which can change engine state.");
		throw Exception("Service performs task which can change engine state");
	} else if (!m_engine) {
		m_log.Warn("Failed to close positions - engine is stopped.");
		throw Exception("Engine is stopped");
	}

	m_engine->ClosePositions();
	m_log.Info("Positions closed.");

}

void EngineServer::Service::OnContextStateChanged(
		const Context::State &state,
		const std::string *updateMessage) {

	const EngineLock lock(m_engineMutex);

	static_assert(Context::numberOfStates == 4, "List changed.");
	switch (state) {

		case Context::STATE_ENGINE_STARTED:

			!updateMessage
				?	m_log.Info("Engine changed state to \"started\".")
				:	m_log.Info(
						"Engine changed state to \"started\": \"%s\".",
						*updateMessage);
			AssertEq(ENGINE_STATE_STARTING, m_engineState);
			Assert(!m_engine);
			m_engineState = ENGINE_STATE_STARTED;

			PublishState();

			break;

		case Context::STATE_DISPATCHER_TASK_STOPPED_GRACEFULLY:
		case Context::STATE_DISPATCHER_TASK_STOPPED_ERROR:

			AssertNe(ENGINE_STATE_STOPPED, m_engineState);

			{
				const char *logRecord
					= state == Context::STATE_DISPATCHER_TASK_STOPPED_GRACEFULLY
					?	!updateMessage
					  	?	"Engine dispatcher task stopped."
						:	"Engine dispatcher task stopped: \"%s\"."
					:	!updateMessage
						?	"Engine dispatcher task stopped with error."
						 :	"Engine dispatcher task stopped with error: \"%s\".";
				if (m_engineState == ENGINE_STATE_STOPPING) {
					!updateMessage
						?	m_log.Debug(logRecord)
						:	m_log.Debug(logRecord, *updateMessage);
				} else if (
						state == Context::STATE_DISPATCHER_TASK_STOPPED_ERROR) {
					!updateMessage
						?	m_log.Warn(logRecord)
						:	m_log.Warn(logRecord, *updateMessage);
				} else {
					!updateMessage
						?	m_log.Info(logRecord)
						:	m_log.Info(logRecord, *updateMessage);
				}
			}

			if (m_engine) {
				if (!m_engineTask.IsActive()) {
					AssertEq(ENGINE_STATE_STARTED, m_engineState);
					// Stopped by engine event, not by command. First dispatcher
					// task stopped...
					m_engineTask.Start(
						[this]() {
							m_log.Debug("Started engine d-tor task.");
							m_engine.reset();
							try {
								const EngineLock lock(m_engineMutex);
								m_engineState = ENGINE_STATE_STOPPED;
								PublishState();
							} catch (...) {
								AssertFailNoException();
								throw;
							}
							m_log.Debug("Finished engine d-tor task.");
						});
					m_engineState = ENGINE_STATE_STOPPING;
					PublishState();
				} else {
					AssertEq(ENGINE_STATE_STOPPING, m_engineState);
				}
			} else {
				if (m_engineState == ENGINE_STATE_STARTING) {
					m_engineState = ENGINE_STATE_STOPPING;
					PublishState();
				}
				AssertEq(ENGINE_STATE_STOPPING, m_engineState);
			}

			break;

		case Context::STATE_STRATEGY_BLOCKED:

			!updateMessage
				?	m_log.Warn("Engine notified about blocked startegy.")
				:	m_log.Warn(
						"Engine notified about blocked startegy: \"%s\".",
						*updateMessage);

			break;

	}

}

bool EngineServer::Service::StoreOperationStartReport(
		size_t recordIndex,
		size_t storeAttemptNo,
		bool dump,
		const uuids::uuid &id,
		const pt::ptime &time,
		const Strategy &strategy) {

	DropCopyRecord record;
	record["id"] = id;
	record["engine_instance_id"] = *m_instanceId;
	record["start_time"] = time;
	record["trading_mode"] = strategy.GetTradingMode();
	record["strategy_id"] = strategy.GetId();

	return StoreRecord(
		&Topics::storeOperationStart,
		recordIndex,
		storeAttemptNo,
		dump,
		std::move(record));

}

bool EngineServer::Service::StoreOperationEndReport(
		size_t recordIndex,
		size_t storeAttemptNo,
		bool dump,
		const uuids::uuid &id,
		const pt::ptime &time,
		const OperationResult &result,
		double pnl,
		const boost::shared_ptr<const FinancialResult> &financialResult) {

	DropCopyRecord record;
	record["id"] = id;
	record["end_time"] = time;
	record["result"] = result;
	record["pnl"] = pnl;
	record["financial_result"] = financialResult;

	return StoreRecord(
		&Topics::storeOperationEnd,
		recordIndex,
		storeAttemptNo,
		dump,
		std::move(record));

}

bool EngineServer::Service::StoreOrder(
		size_t recordIndex,
		size_t storeAttemptNo,
		bool dump,
		const OrderCache &order) {

	DropCopyRecord record;
	record["id"] = order.id;
	if (order.tradingSystemId) {
		record["trade_system_id"] = *order.tradingSystemId;
	}
	if (order.orderTime) {
		record["order_time"] = *order.orderTime;
	}
	if (order.executionTime) {
		record["execution_time"] = *order.executionTime;
	}
	record["status"] = order.status;
	record["operation_id"] = order.operationId;
	if (order.subOperationId) {
		record["sub_operation_id"] = *order.subOperationId;
	}
	record["type"] = ORDER_TYPE_LIMIT;
	record["symbol"] = order.security->GetSymbol().GetSymbol();
	record["source"] = order.tradingSystem->GetTag();
	record["side"] = order.side;
	record["qty"] = order.qty;
	if (order.price) {
		record["price"] = *order.price;
	}
	if (order.timeInForce) {
		record["time_in_force"] = *order.timeInForce;
	}
	record["currency"] = order.currency;
	if (order.minQty) {
		record["min_qty"] = *order.minQty;
	}
	record["executed_qty"] = order.executedQty;
	if (order.bestBidPrice) {
		record["bid_price"] = *order.bestBidPrice;
	}
	if (order.bestBidQty) {
		record["bid_qty"] = *order.bestBidQty;
	}
	if (order.bestAskPrice) {
		record["ask_price"] = *order.bestAskPrice;
	}
	if (order.bestAskQty) {
		record["ask_qty"] = *order.bestAskQty;
	}

	return StoreRecord(
		&Topics::storeOrder,
		recordIndex,
		storeAttemptNo,
		dump,
		std::move(record));

}

bool EngineServer::Service::StoreTrade(
		size_t recordIndex,
		size_t storeAttemptNo,
		bool dump,
		const pt::ptime &time,
		const std::string &tradingSystemTradeId,
		const uuids::uuid &orderId,
		double price,
		const Qty &qty,
		double bestBidPrice,
		const Qty &bestBidQty,
		double bestAskPrice,
		const Qty &bestAskQty) {

	DropCopyRecord record;
	record["time"] = time;
	record["trade_system_id"] = tradingSystemTradeId;
	record["order_id"] = orderId;
	record["price"] = price;
	record["qty"] = qty;
	record["bid_price"] = bestBidPrice;
	record["bid_qty"] = bestBidQty;
	record["ask_price"] = bestAskPrice;
	record["ask_qty"] = bestAskQty;

	return StoreRecord(
		&Topics::storeTrade,
		recordIndex,
		storeAttemptNo,
		dump,
		std::move(record));

}

bool EngineServer::Service::StoreBar(
		size_t recordIndex,
		size_t storeAttemptNo,
		bool dump,
		const BarCache &bar) {

	DropCopyRecord record;
	record["engine"] = *m_instanceId;
	record["source"] = bar.security->GetSource().GetTag();
	record["symbol"] = bar.security->GetSymbol().GetSymbol();
	record["time"] = ConvertToMicroseconds(bar.time);
	AssertLt(0, bar.size.total_seconds());
	record["size"] = int64_t(bar.size.total_seconds());
	record["open"] = bar.security->DescalePrice(bar.open);
	record["close"] = bar.security->DescalePrice(bar.close);
	record["high"] = bar.security->DescalePrice(bar.high);
	record["low"] = bar.security->DescalePrice(bar.low);

	return StoreRecord(
		&Topics::storeBar,
		recordIndex,
		storeAttemptNo,
		dump,
		std::move(record));

}

bool EngineServer::Service::StoreAbstractDataPoint(
		size_t recordIndex,
		size_t storeAttemptNo,
		bool dump,
		const DropCopy::AbstractDataSourceId &source,
		const pt::ptime &time,
		double value) {

	DropCopyRecord record;
	{
		const intmax_t sourceInt = source;
		record["source"] = sourceInt;
	}
	record["time"] = ConvertToMicroseconds(time);
	record["value"] = value;

	return StoreRecord(
		&Topics::storeAbstractDataPoint,
		recordIndex,
		storeAttemptNo,
		dump,
		std::move(record));

}

bool EngineServer::Service::StoreRecord(
		const std::string Topics::*topic,
		size_t recordIndex,
		size_t storeAttemptNo,
		bool dump,
		const DropCopyRecord &&record) {

	if (dump) {
		DumpRecord(topic, recordIndex, storeAttemptNo, std::move(record));
		return true;
	}

	AssertLt(0, storeAttemptNo);
	if (storeAttemptNo == 1) {
		m_dropCopy.GetDataLog().Info(
			"store\t%1%\t%2%\t%3%\t%4%",
			recordIndex,
			storeAttemptNo,
			m_connection->topics.*topic,
			ConvertToLogString(record));
	}

	boost::future<autobahn::wamp_call_result> callFuture;
	{

		const ConnectionLock lock(m_connectionMutex);
		if (!m_connection) {
			m_dropCopy.GetDataLog().Warn(
				"no connection\t%1%\t%2%\t%3%",
				recordIndex,
				storeAttemptNo,
				m_connection->topics.*topic);
			return false;
		}

		try {
			callFuture = m_connection->session->call(
				m_connection->topics.*topic,
				std::make_tuple(),
				record,
				m_config.callOptions);
		} catch (const std::exception &ex) {
			m_log.Error(
				"Failed to call TWS to store Drop Copy record: \"%1%\".",
				ex.what());
			m_dropCopy.GetDataLog().Error(
				"store error\t%1%\t%2%\t%3%\t%4%",
				recordIndex,
				storeAttemptNo,
				m_connection->topics.*topic,
				ex.what());
			return false;
		}

	}

	try {
		if (callFuture.get().argument<bool>(0)) {
			m_dropCopy.GetDataLog().Info(
				"stored\t%1%\t%2%\t%3%",
				recordIndex,
				storeAttemptNo,
				m_connection->topics.*topic);
		} else {
			m_log.Error(
				"Failed to store Drop Copy record: TWS returned error."
					" Record %1% will be skipped at attempt %2%.",
				recordIndex,
				storeAttemptNo);
			DumpRecord(topic, recordIndex, storeAttemptNo, std::move(record));
		}
		return true;
	} catch (const std::exception &ex) {
		m_log.Error("Failed to store Drop Copy record: \"%1%\".", ex.what());
		m_dropCopy.GetDataLog().Error(
			"store error\t%1%\t%2%\t%3%\t%4%",
			recordIndex,
			storeAttemptNo,
			m_connection->topics.*topic,
			ex.what());
	}

	return false;

}

void EngineServer::Service::DumpRecord(
		const std::string Topics::*topic,
		size_t recordIndex,
		size_t storeAttemptNo,
		const DropCopyRecord &&record) {
	m_dropCopy.GetDataLog().Error(
		"dump\t%1%\t%2%\t%3%\t%4%",
		recordIndex,
		storeAttemptNo,
		m_connection->topics.*topic,
		ConvertToLogString(record));
}

bool EngineServer::Service::StoreBook(
		size_t /*recordIndex*/,
		size_t /*storeAttemptNo*/,
		bool dump,
		const Security &security,
		const PriceBook &book) {

	if (dump) {
		return true;
	}

	boost::future<autobahn::wamp_call_result> callFuture;
	{

		const ConnectionLock lock(m_connectionMutex);
		if (!m_connection) {
			return true;
		}

		{
			auto it = m_bookSentTime.find(&security);
			if (it == m_bookSentTime.cend()) {
				m_bookSentTime.emplace(&security, book.GetTime());
			} else {
				AssertLe(it->second, book.GetTime());
				if (book.GetTime() - it->second <= pt::seconds(1)) {
					return true;
				}
				it->second = book.GetTime();
			}
		}

		try {
			callFuture = m_connection->session->call(
				m_connection->topics.storeBook,
				std::make_tuple(
					*m_instanceId,
					security.GetSource().GetTag(),
					security.GetSymbol().GetSymbol(),
					ConvertToMicroseconds(book.GetTime()),
					book.GetBid(),
					book.GetAsk()),
				m_config.callOptions);
		} catch (const std::exception &ex) {
			m_log.Error(
				"Failed to call TWS to store Price Book into Drop Copy storage"
					 ": \"%1%\".",
				ex.what());
			return true;
		}

	}

	try {
		callFuture.wait();
	} catch (const std::exception &ex) {
		m_log.Error(
			"Failed to store Price Book into Drop Copy storage"
				": \"%1%\".",
			ex.what());
	}

	return true;

}

EngineServer::Service::DropCopy::AbstractDataSourceId
EngineServer::Service::RegisterAbstractDataSource(
		const uuids::uuid &instance,
		const uuids::uuid &type,
		const std::string &name) {

	DropCopyRecord record;
	record["instance"] = instance;
	record["type"] = type;
	record["engine"] = *m_instanceId;
	record["name"] = name;

	const auto &topic = m_connection->topics.registerAbstractDataSource;

	m_dropCopy.GetDataLog().Info(
		"store\t%1%\t%2%\t%3%\t%4%",
		0,
		1,
		topic,
		ConvertToLogString(record));

	boost::future<autobahn::wamp_call_result> callFuture;
	{

		const ConnectionLock lock(m_connectionMutex);
		if (!m_connection) {
			m_dropCopy.GetDataLog().Warn(
				"no connection\t%1%\t%2%\t%3%",
				0,
				1,
				topic);
			throw DropCopy::Exception("Has no TWS connection");
		}

		try {
			callFuture = m_connection->session->call(
				topic,
				std::make_tuple(),
				record,
				m_config.callOptions);
		} catch (const std::exception &ex) {
			m_log.Error(
				"Failed to call TWS to register Abstract Data Source: \"%1%\".",
				ex.what());
			m_dropCopy.GetDataLog().Error(
				"store error\t%1%\t%2%\t%3%\t%4%",
				0,
				1,
				topic,
				ex.what());
			throw DropCopy::Exception("Failed to call TWS");
		}

	}

	try {
		const auto &result
			= callFuture.get().argument<DropCopy::AbstractDataSourceId>(0);
		if (result != 0) {
			m_dropCopy.GetDataLog().Info(
				"result\t%1%\t%2%\t%3%\t%4%",
				0,
				1,
				topic,
				result);
			return result;
		}
	} catch (const std::exception &ex) {
		m_log.Error(
			"Failed to call TWS to register Abstract Data Source: \"%1%\".",
			ex.what());
		m_dropCopy.GetDataLog().Error(
			"store error\t%1%\t%2%\t%3%\t%4%",
			0,
			1,
			topic,
			ex.what());
	}

	m_log.Error(
		"Failed to call TWS to register Abstract Data Source"
				": TWS returned error.");
	throw DropCopy::Exception("Failed to call TWS");

}

////////////////////////////////////////////////////////////////////////////////
