/**************************************************************************
 *   Created: 2012/08/28 01:33:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#include "Prec.hpp"
#include "Gateway.hpp"
#include "Core/Security.hpp"
#include "Core/Settings.hpp"
#include "Common/Exception.hpp"

#define TRADER_LIGHTSPEED_GATEWAY "Lightspeed Gateway"
#define TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX TRADER_LIGHTSPEED_GATEWAY ": "

namespace io = boost::asio;
namespace pt = boost::posix_time;
namespace accum = boost::accumulators;

using namespace Trader::Interaction::Lightspeed;

//////////////////////////////////////////////////////////////////////////

Gateway::Connection::Connection(
			size_t socketBufferSize,
			size_t messagesBufferSize,
			const boost::posix_time::time_duration &timeout)
		: socket(ioService),
		socketBuffer(socketBufferSize),
		messagesBuffer(messagesBufferSize),
		stage(STAGE_CONNECTING),
		seqnumber(0),
		timeout(timeout),
		lastToken(0),
		orderMessagesFromLastStatCount() {
	//...//
}

Gateway::Connection::~Connection() {
	Log::Debug(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "destroying connection...");
	socket.close();
	ioService.stop();
	tasks.join_all();
}

//////////////////////////////////////////////////////////////////////////

Gateway::Gateway() {
	//...//
}

Gateway::~Gateway() {
	//...//
}

void Gateway::HandleConnect(
			Connection &connection,
			const boost::system::error_code &error,
			Endpoint &endpoint,
			ResolverIterator endpointIterator) {
	AssertEq(STAGE_CONNECTING, connection.stage);
	std::unique_ptr<Endpoint> endpointHolder(&endpoint);
	const Lock lock(m_mutex);
	if (!error) {
		Log::Debug(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connected...");
		AssertEq(STAGE_CONNECTING, connection.stage);
		connection.stage = STAGE_CONNECTED;
		StartInitialDataReading(connection);
	} else if (endpointIterator != ResolverIterator()) {
		Log::Debug(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection to next endpoint...");
		connection.socket.close();
		std::unique_ptr<Endpoint> endpoint(new Endpoint(*endpointIterator));
		connection.socket.async_connect(
			*endpoint,
			boost::bind(
				&Gateway::HandleConnect,
				this,
				boost::ref(connection),
				io::placeholders::error,
				boost::ref(*endpoint),
				++endpointIterator));
		endpoint.release();
	} else {
		Log::Debug(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "no more endpoints...");
		m_condition.notify_all();
	}
}

void Gateway::HandleResolve(
			Connection &connection,
			const boost::system::error_code &error,
			ResolverIterator endpointIterator) {

	AssertEq(STAGE_CONNECTING, connection.stage);

	const Lock lock(m_mutex);
	if (error) {
		Log::Error(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "server endpoint RESOLVE ERROR.");
		m_condition.notify_all();
		return;
	}

	Log::Debug(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "server endpoint resolved...");

	std::unique_ptr<Endpoint> endpoint(new Endpoint(*endpointIterator));
	connection.socket.async_connect(
		*endpoint,
		boost::bind(
			&Gateway::HandleConnect,
			this,
			boost::ref(connection),
			io::placeholders::error,
			boost::ref(*endpoint),
			++endpointIterator));
	endpoint.release();

}

void Gateway::StartReading() {
	Assert(m_connection);
	AssertGe(m_connection->stage, STAGE_CONNECTED);
	StartReading(*m_connection);
}

void Gateway::StartReading(Connection &connection) {
	connection.socket.async_read_some(
		io::buffer(&connection.socketBuffer[0], connection.socketBuffer.size()),
		boost::bind(
			&Gateway::HandleRead,
			this,
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void Gateway::StartInitialDataReading(Connection &connection) {
	connection.socket.async_read_some(
		io::buffer(&connection.socketBuffer[0], connection.socketBuffer.size()),
		boost::bind(
			&Gateway::HandleInitialDataRead,
			this,
			boost::ref(connection),
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void Gateway::Connect(const IniFile &ini, const std::string &section) {

	std::unique_ptr<Connection> connection;
	{
		const auto socketBufferSize
			= ini.ReadTypedKey<size_t>(section, "socket_buffer_size_bytes");
		const auto messagesBufferSize
			= ini.ReadTypedKey<size_t>(section, "messages_buffer_size_bytes");
		const auto timeout
			= pt::milliseconds(ini.ReadTypedKey<size_t>(section, "timeout_msec"));
		connection.reset(
			new Connection(socketBufferSize, messagesBufferSize, timeout));
	}
	const auto heartbeatPeriod
		= pt::seconds(ini.ReadTypedKey<size_t>(section, "heartbeat_period_sec"));
	Log::Info(
		TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX
			"socket_buffer_size_bytes = %1%; messages_buffer_size_bytes = %2%;"
				" timeout_msec = %3%; heartbeat_period_sec = %4%;",
		connection->socketBuffer.size(),
		connection->messagesBuffer.capacity(),
		connection->timeout,
		heartbeatPeriod);

	const auto ipAddress = ini.ReadKey(section, "ip_address", false);
	const auto port = ini.ReadTypedKey<size_t>(section, "port");
	Log::Info(
		TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connecting to \"%1%:%2%...",
		ipAddress,
		port);

	{

		Lock lock(m_mutex);
		Assert(!m_connection);
		if (m_connection) {
			Log::Warn(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection already exists.");
			return;
		}

		Proto::resolver::query query(
			Proto::v4(),
			ipAddress,
			boost::lexical_cast<std::string>(port));
		Resolver resolver(connection->ioService);

		resolver.async_resolve(
			query,
			boost::bind(
				&Gateway::HandleResolve,
				this,
				boost::ref(*connection),
				io::placeholders::error,
				io::placeholders::iterator));

		{
			Connection *connectionPtr = connection.get();
			connection->tasks.create_thread(
				[connectionPtr]() {
					Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "started reading task.");
					try {
						connectionPtr->ioService.run();
					} catch (...) {
						AssertFailNoException();
						throw;
					}
					Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "stopped reading task.");
				});
		}

		m_condition.timed_wait(lock, boost::get_system_time() + connection->timeout);
		if (connection->stage != STAGE_HANDSHAKED) {
			AssertLt(connection->stage, STAGE_HANDSHAKED);
			Log::Error(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "CONNECTION FAILED or no handshake received.");
			throw ConnectError();
		}

		SendLoginRequest(
			*connection,
			ini.ReadKey(section, "login", false),
			ini.ReadKey(section, "password", false));
		m_condition.timed_wait(lock, boost::get_system_time() + connection->timeout);
		if (connection->stage != STAGE_LOGGED_ON) {
			AssertLt(connection->stage, STAGE_LOGGED_ON);
			if (connection->stage != STAGE_CLOSED_BY_LOGIN_REJECTED) {
				Log::Error(
					TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connected,"
						" but logging timeout occurred");
			} else {
				Log::Error(
					TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connected,"
						" but logging request has been REJECTED.");
			}
			throw ConnectError();
		}

		const boost::function<void (const pt::time_duration &)> heartbeatTask
			= [this](const pt::time_duration &heartbeatPeriod) {
					Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "started heartbeat task.");
					try {
						Lock lock(m_mutex);
						for ( ; ; ) {
							SendHeartbeat(*m_connection);
							m_condition.timed_wait(
								lock,
								boost::get_system_time() + heartbeatPeriod);
						}
					} catch (const ConnectionDoesntExistError &) {
						//...//
					} catch (...) {
						AssertFailNoException();
						throw;
					}
					Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "stopped heartbeat task.");
				};
		connection->tasks.create_thread(boost::bind(heartbeatTask, heartbeatPeriod));

	}

	m_connection.reset(connection.release());
	Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "CONNECTED and LOGGED ON.");

}

bool Gateway::IsReadingClosed(
			Connection &connection,
			const boost::system::error_code &error,
			size_t size)
		const {
	AssertGe(connection.stage, STAGE_CONNECTED);
	if (size == 0) {
		connection.stage = STAGE_CLOSED_GRACEFULLY;
		Log::Info(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection CLOSED.");
		return true;
	} else if (error) {
		connection.stage = STAGE_CLOSED_BY_READ_ERROR;
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection CLOSED with ERROR: \"%1%\" (%2%).",
			error.message(),
			error.value());
		return true;
	}
	return connection.stage < STAGE_CONNECTING;
}

void Gateway::HandleRead(const boost::system::error_code &error, size_t size) {
	const Lock lock(m_mutex);
	if (!m_connection || IsReadingClosed(*m_connection, error, size)) {
		return;
	}
	AssertGe(m_connection->stage, STAGE_CONNECTED);
	try {
		if (!HandleRead(*m_connection, error, size, lock)) {
			return;
		}
	} catch (const ProtocolError &ex) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "PROTOCOL error: \"%1%\""
				" for message \"%2%\" (stage: %3%).",
			ex.what(),
			ex.GetMessage(),
			ex.GetStage());
	} catch (const MessageError &ex) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "error \"%1%\""
				" in message \"%2%\" (stage: %3%).",
			ex.what(),
			ex.GetMessage());
	}
	StartReading();
}

void Gateway::HandleInitialDataRead(
			Connection &connection,
			const boost::system::error_code &error,
			size_t size) {
	const Lock lock(m_mutex);
	Assert(!m_connection);
	AssertGe(connection.stage, STAGE_CONNECTED);
	AssertLt(connection.stage, STAGE_LOGGED_ON);
	m_condition.notify_all();
	if (IsReadingClosed(connection, error, size)) {
		return;
	}
	try {
		if (!HandleRead(connection, error, size, lock)) {
			return;
		}
	} catch (const ProtocolError &ex) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "PROTOCOL error: \"%1%\""
				" for message \"%2%\" (stage: %3%).",
			ex.what(),
			ex.GetMessage(),
			ex.GetStage());
	} catch (const MessageError &ex) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "error \"%1%\""
				" in message \"%2%\" (stage: %3%).",
			ex.what(),
			ex.GetMessage());
	}
	if (connection.stage >= STAGE_CONNECTED) {
		if (connection.stage < STAGE_LOGGED_ON) {
			StartInitialDataReading(connection);
		} else {
			StartReading(connection);
		}
	}
}

bool Gateway::HandleRead(
			Connection &connection,
			const boost::system::error_code &error,
			size_t size,
			const Lock &) {

	if (IsReadingClosed(connection, error, size)) {
		return false;
	}

	Assert(connection.socketBuffer.size() >= size);
	const auto oldSize = connection.messagesBuffer.size();
	if (connection.messagesBuffer.capacity() < oldSize + size) {
		Log::Warn(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "exceeded the internal buffer"
				" (current capacity: %2%, new data size: %3%)."
				" Buffer will be reallocated.",
			connection.messagesBuffer.capacity(),
			size);
		MessagesBuffer newBuffer(oldSize + size);
		newBuffer.resize(oldSize);
		memcpy(&newBuffer[0], &connection.messagesBuffer[0], oldSize);
		newBuffer.swap(connection.messagesBuffer);
	}
	connection.messagesBuffer.resize(oldSize + size);
	memcpy(&connection.messagesBuffer[oldSize], &connection.socketBuffer[0], size);

	for ( ; ; ) {
		MessagesBuffer::iterator end = std::find(
			connection.messagesBuffer.begin() + oldSize,
			connection.messagesBuffer.end(),
			'\n');
		if (end == connection.messagesBuffer.end()) {
			break;
		}
		try {
			HandleMessage(TsMessage(connection.messagesBuffer.begin(), end), connection);
		} catch (const TsMessage::Error &ex) {
			connection.messagesBuffer.erase(connection.messagesBuffer.begin(), ++end);
			throw ProtocolError(ex.what(), ex.GetSubject(), connection.stage);
		}
		connection.messagesBuffer.erase(connection.messagesBuffer.begin(), ++end);
	}

	return true;

}

void Gateway::HandleMessage(const TsMessage &message, Connection &connection) {

	switch (message.GetType()) {
		case TsMessage::TYPE_DEBUG:
			HandleDebug(message, connection);
			break;
		case TsMessage::TYPE_LOGIN_ACCEPTED:
			HandleLoginAccepted(message, connection);
			break;
		case TsMessage::TYPE_LOGIN_REJECTED:
			HandleLoginRejected(message, connection);
			break;
		case TsMessage::TYPE_HEARTBEAT:
			break;
		case TsMessage::TYPE_ORDER_ACCEPTED:
			HandleOrderAccepted(message, connection);
			break;
		case TsMessage::TYPE_ORDER_REJECTED:
			HandleOrderReject(message, connection);
			break;
		case TsMessage::TYPE_ORDER_CANCELED:
			HandleOrderCanceled(message, connection);
			break;
		case TsMessage::TYPE_ORDER_EXECUTED:
			HandleOrderExecuted(message, connection);
			break;
		default:
			throw MessageError("Unknown trade system message", message);
	}

}

void Gateway::StatOrders(Connection &connection) throw() {
	if (++connection.orderMessagesFromLastStatCount < 100)  {
		return;
	}
	connection.orderMessagesFromLastStatCount = 0;
	Log::Debug(
		TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "active orders storage size: %1%.",
		connection.orders.size());
}

void Gateway::HandleOrderAccepted(const TsMessage &message, Connection &connection) {

	AssertEq(TsMessage::TYPE_ORDER_ACCEPTED, message.GetType());

	const auto token = Token(message.GetNumericField(9, 16));
	auto &index = connection.orders.get<ByToken>();
	const auto order = index.find(token);
	if (order == index.end()) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX
				"unknown accepted order (token: %1%, order ID: %2%).",
			token,
			message.GetFieldAsString(25, 9));
		throw MessageError("Unknown accepted order", message);
	}

	order->callback(token, ORDER_STATUS_SUBMITTED, 0, 0, 0, 0);

	StatOrders(connection);

}

void Gateway::HandleOrderReject(const TsMessage &message, Connection &connection) {

	AssertEq(TsMessage::TYPE_ORDER_REJECTED, message.GetType());

	const auto token = Token(message.GetNumericField(9, 16));
	const auto reason = message.GetOrderRejectReasonField(25);

	auto &index = connection.orders.get<ByToken>();
	const auto order = index.find(token);
	if (order == index.end()) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX
				"unknown rejected order with token: %1% (reject reason: \"%2%\").",
			token,
			reason);
		throw MessageError("Unknown rejected order", message);
	} else {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX
				"order %1% rejected with reason: \"%2%\".",
			token,
			reason);
	}

	const auto callback = order->callback;
	index.erase(order);
	callback(token, ORDER_STATUS_ERROR, 0, 0, 0, 0);

	StatOrders(connection);

}

void Gateway::HandleOrderCanceled(const TsMessage &message, Connection &connection) {

	AssertEq(TsMessage::TYPE_ORDER_CANCELED, message.GetType());

	const auto token = Token(message.GetNumericField(9, 16));
	auto &index = connection.orders.get<ByToken>();
	const auto order = index.find(token);
	if (order == index.end()) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX
				"unknown order canceled (token: %1%, shares canceled: \"%2%\", reason: \"%3%\").",
			token,
			message.GetNumericField(25, 6), // 25 - canceled qty
			message.GetOrderCancelReasonField(31));
		throw MessageError("Unknown order canceled", message);
	}

	AssertLe(message.GetNumericField(25, 6), order->initialQty); // 25 - canceled qty
	AssertLe(message.GetNumericField(25, 6), order->initialQty - order->executedQty); // 25 - canceled qty

	const auto reason = message.GetCharField(31);
	if (reason != 'U') {
		Log::Warn(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX
				"order %1% canceled with unusual reason: \"%2%\" (shares canceled: %3%).",
			token,
			message.GetOrderCancelReasonField(31),
			message.GetNumericField(25, 6));
	}

	const auto callback = order->callback;
	index.erase(order);
	callback(token, ORDER_STATUS_CANCELLED, 0, 0, 0, 0);

	StatOrders(connection);

}

void Gateway::HandleOrderExecuted(const TsMessage &message, Connection &connection) {

	AssertEq(TsMessage::TYPE_ORDER_EXECUTED, message.GetType());

	const auto token = Token(message.GetNumericField(9, 16));
	const auto executedQty = OrderQty(message.GetNumericField(25, 6));
	const auto executionPrice = message.GetPriceField(31, 6);

	auto &index = connection.orders.get<ByToken>();
	const auto order = index.find(token);
	if (order == index.end()) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX
				"unknown order executed (token: %1%, qty: \"%2%\", price: \"%3%\").",
			token,
			executedQty,
			executionPrice);
		throw MessageError("Unknown order executed", message);
	}

	AssertLe(executedQty, order->initialQty);
	AssertLe(executedQty, order->initialQty - order->executedQty);
	if (executedQty + order->executedQty < order->initialQty) {
		index.modify(order, Order::Modifers::Execution(executedQty, executionPrice));
		order->callback(
			token,
			ORDER_STATUS_FILLED,
			executedQty,
			order->initialQty - order->executedQty,
			accum::mean(order->avgExecutionPrice),
			executionPrice);
	} else {
		AssertEq(order->initialQty, executedQty + order->executedQty);
		auto avgExecutionPrice = order->avgExecutionPrice;
		avgExecutionPrice(executionPrice);
		const auto callback = order->callback;
		index.erase(order);
		callback(
			token,
			ORDER_STATUS_FILLED,
			executedQty,
			0,
			accum::mean(avgExecutionPrice),
			executionPrice);
	}

	StatOrders(connection);

}

void Gateway::HandleDebug(const TsMessage &message, Connection &connection) {
	AssertEq(TsMessage::TYPE_DEBUG, message.GetType());
	if (!m_connection) {
		AssertEq(STAGE_CONNECTED, connection.stage);
		connection.stage = STAGE_HANDSHAKED;
		Log::Info(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "server protocol \"%1%\".",
			message.GetAsString(true));
	} else {
		Log::DebugEx(
			[&message]() -> boost::format {
				boost::format result(TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "debug packet: \"%1%\".");
				result % message.GetAsString(true);
				return result;
			});
	}
}

void Gateway::HandleLoginAccepted(const TsMessage &message, Connection &connection) {
	AssertEq(TsMessage::TYPE_LOGIN_ACCEPTED, message.GetType());
	if (!m_connection) {
		AssertEq(STAGE_HANDSHAKED, connection.stage);
		connection.stage = STAGE_LOGGED_ON;
		Assert(connection.seqnumber == 0);
		const std::string session = message.GetAlphanumField(1, 10);
		connection.seqnumber = message.GetNumericField(11, 10);
		Log::Info(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "session: \"%1%\", seqnumber: %2%.",
			session,
			connection.seqnumber);
	} else {
		throw ProtocolError(
			"Unexpected Login Accepted packet",
			message.GetBegin(),
			message.GetEnd(),
			connection.stage);
	}
}

void Gateway::HandleLoginRejected(const TsMessage &message, Connection &connection) {
	AssertEq(TsMessage::TYPE_LOGIN_REJECTED, message.GetType());
	if (!m_connection) {
		AssertEq(0, connection.seqnumber);
		AssertEq(STAGE_HANDSHAKED, connection.stage);
		connection.stage = STAGE_CLOSED_BY_LOGIN_REJECTED;
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "login rejected with reason: %1% (code: %2%).",
			message.GetLoginRejectReasonField(1),
			message.GetCharField(1));
	} else {
		throw ProtocolError(
			"Unexpected Login Rejected packet",
			message.GetBegin(),
			message.GetEnd(),
			connection.stage);
	}
}

void Gateway::SendLoginRequest(
			Connection &connection,
			const std::string &login,
			const std::string &password) {
	Log::Info(
		TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "sending longing request for \"%1%\"...",
		login);
	try {
		boost::shared_ptr<ClientMessage> message(
			new ClientMessage(ClientMessage::TYPE_LOGIN_REQUEST));
		message->AppendField(login, 6);
		message->AppendField(password, 10);
		message->AppendSpace(10);
		message->AppendField(ClientMessage::Numeric(0), 10);
		AssertEq(37, message->GetMessageLogicalLen());
		Send(message, connection);
	} catch (const ClientMessage::Error &ex) {
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "failed to send Login request: \"%1%\" (\"%2%\").",
			ex.what(),
			ex.GetSubject());
		throw SendingError();
	}
}

void Gateway::SendHeartbeat(Connection &connection) {
	boost::shared_ptr<ClientMessage> message(new ClientMessage(ClientMessage::TYPE_HEARTBEAT));
	AssertEq(1, message->GetMessageLogicalLen());
	Send(message, connection);
}

Gateway::OrderId Gateway::SendOrder(
			const Security &security,
			ClientMessage::BuySellIndicator buySell,
			OrderQty qty,
			OrderPrice price,
			ClientMessage::Numeric timeInForce,
			const OrderStatusUpdateSlot &callback) {

	const Token token = Interlocking::Increment(m_connection->lastToken);

	boost::shared_ptr<ClientMessage> message(new ClientMessage(ClientMessage::TYPE_NEW_ORDER));
	message->AppendFieldAsAlphanum(token, 16);
	message->AppendField('A');
	message->AppendField(buySell);
	message->AppendField(qty, 6);
	message->AppendField(qty, 6);
	message->AppendField(security.GetSymbol(), 6);
	message->AppendField(security.DescalePrice(price), 10);
	message->AppendField(.0, 5);
	message->AppendField(timeInForce, 5);
	message->AppendSpace(10);
	AssertEq(67, message->GetMessageLogicalLen());

	const Lock lock(m_mutex);
	const auto pos = m_connection->orders.insert(
		Order(OrderId(token), callback, qty));
	try {
		Send(message, *m_connection);
	} catch (...) {
		// copy ctor and swap is too expensive here...
		m_connection->orders.erase(pos.first);
		throw;
	}

	StatOrders(*m_connection);

	return Gateway::OrderId(token);

}

void Gateway::Send(
			boost::shared_ptr<ClientMessage> message,
			Connection &connection) {
	if (connection.stage < STAGE_CONNECTED) {
		throw ConnectionDoesntExistError("Connection lost");
	}
	message->AppendField('\n');
	io::async_write(
		connection.socket,
		message->GetMessage(),
        boost::bind(
			&Gateway::HandleWrite,
			this,
			message,
			io::placeholders::error,
			io::placeholders::bytes_transferred));
}

void Gateway::HandleWrite(
			boost::shared_ptr<const ClientMessage>,
			const boost::system::error_code &error,
			size_t /*size*/) {
	if (!error) {
		return;
	}
	{
		const Lock lock(m_mutex);
		if (!m_connection) {
			return;
		}
		AssertLe(STAGE_CONNECTED, m_connection->stage);
		m_connection->stage = STAGE_CLOSED_BY_WRITE_ERROR;
		Log::Error(
			TRADER_LIGHTSPEED_GATEWAY_LOG_PREFFIX "connection CLOSED with write ERROR: \"%1%\" (%2%).",
			error.message(),
			error.value());
	}
	m_condition.notify_all();
}

bool Gateway::IsCompleted(const Security &) const {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::SellAtMarketPrice(
			const Security &security,
			OrderQty qty,
			const OrderStatusUpdateSlot &callback) {
	return SendOrder(
		security,
		ClientMessage::BUY_SELL_INDICATOR_SELL_LONG,
		qty,
		0,
		99999,
		callback);
}

Gateway::OrderId Gateway::Sell(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::SellAtMarketPriceWithStopPrice(
			const Security &,
			OrderQty,
			OrderPrice /*stopPrice*/,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::SellOrCancel(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::BuyAtMarketPrice(
			const Security &security,
			OrderQty qty,
			const OrderStatusUpdateSlot &callback) {
	return SendOrder(
		security,
		ClientMessage::BUY_SELL_INDICATOR_BUY,
		qty,
		0,
		99999,
		callback);
}

Gateway::OrderId Gateway::Buy(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::BuyAtMarketPriceWithStopPrice(
			const Security &,
			OrderQty,
			OrderPrice /*stopPrice*/,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

Gateway::OrderId Gateway::BuyOrCancel(
			const Security &,
			OrderQty,
			OrderPrice,
			const OrderStatusUpdateSlot &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

void Gateway::CancelOrder(OrderId) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}

void Gateway::CancelAllOrders(const Security &) {
	AssertFail("Doesn't implemented.");
	throw Exception("Doesn't implemented");
}
